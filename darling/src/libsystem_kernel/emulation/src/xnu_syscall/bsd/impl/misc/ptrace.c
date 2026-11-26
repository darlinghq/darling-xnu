#include <darling/emulation/xnu_syscall/bsd/impl/misc/ptrace.h>

#include <sys/errno.h>
#include <stddef.h>
#include <sys/signal.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/kill.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/linux_premigration/signal/sigexc.h>
#include <darling/emulation/xnu_syscall/bsd/impl/process/wait4.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/read.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getppid.h>

#include <darlingserver/rpc.h>

// faster than sys_getpid(), because it caches the PID
extern int getpid(void);

#define TRACEE_MAX 64
#define LINUX_POLLIN 0x0001

struct linux_pollfd_tracee {
	int fd;
	short events;
	short revents;
};

struct linux_timespec_tracee {
	long tv_sec;
	long tv_nsec;
};

// Processes this process attached to; 0 marks a free slot.
static int tracees[TRACEE_MAX];

static void tracee_add(int pid)
{
	for (int i = 0; i < TRACEE_MAX; i++)
	{
		if (__atomic_load_n(&tracees[i], __ATOMIC_ACQUIRE) == pid)
			return;
	}
	for (int i = 0; i < TRACEE_MAX; i++)
	{
		int expected = 0;
		if (__atomic_compare_exchange_n(&tracees[i], &expected, pid, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
			return;
	}
}

static void tracee_remove(int pid)
{
	for (int i = 0; i < TRACEE_MAX; i++)
	{
		int expected = pid;
		__atomic_compare_exchange_n(&tracees[i], &expected, 0, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
	}
}

// Reads the exit status (Linux wait status format) of a process that has just exited
// from /proc. Returns 0 if it cannot be known, for example because its real parent
// already reaped it.
static int tracee_exit_status(int pid)
{
	char path[32] = "/proc/";
	int len = 6;
	char digits[12];
	int nd = 0;
	for (unsigned v = (unsigned) pid; v != 0; v /= 10)
		digits[nd++] = '0' + (v % 10);
	while (nd > 0)
		path[len++] = digits[--nd];
	const char suffix[] = "/stat";
	for (int i = 0; i < (int) sizeof(suffix); i++)
		path[len++] = suffix[i];

	int fd = sys_open_nocancel(path, BSD_O_RDONLY, 0);
	if (fd < 0)
		return 0;
	char buf[1024];
	int n = sys_read_nocancel(fd, buf, sizeof(buf) - 1);
	close_internal(fd);
	if (n <= 0)
		return 0;
	buf[n] = '\0';

	// the command name may contain anything, so count fields from the last ')'
	char* p = buf + n;
	while (p > buf && *p != ')')
		p--;
	if (*p != ')')
		return 0;
	p++;

	// fields after the name start at field 3 (state); exit_code is field 52
	int field = 2;
	char state = 0;
	long exit_code = 0;
	while (*p != '\0')
	{
		while (*p == ' ')
			p++;
		if (*p == '\0')
			break;
		field++;
		if (field == 3)
			state = *p;
		if (field == 52)
		{
			for (; *p >= '0' && *p <= '9'; p++)
				exit_code = exit_code * 10 + (*p - '0');
			break;
		}
		while (*p != ' ' && *p != '\0')
			p++;
	}
	return (state == 'Z' && field == 52) ? (int) exit_code : 0;
}

long ptrace_wait_tracee(int pid, int* linux_status, int nohang)
{
	int candidates[TRACEE_MAX];
	struct linux_pollfd_tracee fds[TRACEE_MAX];
	int count = 0;

	if (pid != -1 && pid <= 0)
		return -LINUX_ECHILD;

	for (int i = 0; i < TRACEE_MAX; i++)
	{
		int p = __atomic_load_n(&tracees[i], __ATOMIC_ACQUIRE);
		if (p != 0 && (pid == -1 || pid == p))
			candidates[count++] = p;
	}
	if (count == 0)
		return -LINUX_ECHILD;

	int nfds = 0;
	for (int i = 0; i < count; i++)
	{
		int pidfd = LINUX_SYSCALL(__NR_pidfd_open, candidates[i], 0);
		if (pidfd < 0)
		{
			// gone already, and reaped by its real parent: nothing left to learn about it
			for (int j = 0; j < nfds; j++)
				LINUX_SYSCALL(__NR_close, fds[j].fd);
			tracee_remove(candidates[i]);
			*linux_status = 0;
			return candidates[i];
		}
		candidates[nfds] = candidates[i];
		fds[nfds].fd = pidfd;
		fds[nfds].events = LINUX_POLLIN;
		fds[nfds].revents = 0;
		nfds++;
	}

	struct linux_timespec_tracee zero = { 0, 0 };
	long ret = LINUX_SYSCALL(__NR_ppoll, fds, nfds, nohang ? &zero : NULL, NULL, 8);

	int ready = -1;
	for (int i = 0; i < nfds; i++)
	{
		if (ready < 0 && fds[i].revents != 0)
			ready = i;
		LINUX_SYSCALL(__NR_close, fds[i].fd);
	}

	if (ret < 0)
		return ret;
	if (ret == 0 || ready < 0)
		return 0;

	tracee_remove(candidates[ready]);
	*linux_status = tracee_exit_status(candidates[ready]);
	return candidates[ready];
}

long sys_ptrace(int request, int pid, void* addr, int data)
{
	const char* cmd = NULL;
	int ret = -EINVAL;

	switch (request)
	{
		case XNU_PT_PTRACE_ME:
		{
			// ret = LINUX_SYSCALL(__NR_ptrace, LINUX_PTRACE_TRACEME, pid, addr, data);
			
			// if (ret < 0)
			// 	ret = errno_linux_to_bsd(ret);

			// Use darlingserver mechanisms to set a tracing task (->disallow anyone else to attach)
			ret = dserver_rpc_set_tracer(0, sys_getppid());
			if (ret < 0) {
				__simple_printf("dserver_rpc_set_tracer failed internally: %d", ret);
				__simple_abort();
			}
			ret = errno_linux_to_bsd(-ret);
			
			//return ret;
			cmd = "PT_PTRACE_ME"; break;
		}
		case XNU_PT_KILL:
		{
			ret = sys_kill(pid, SIGKILL, 1);
			// ret = LINUX_SYSCALL(__NR_ptrace, LINUX_PTRACE_KILL, pid, addr, data);
			// if (ret < 0)
			// 	ret = errno_linux_to_bsd(ret);

			// return ret;
			cmd = "PT_KILL"; break;
		}
		case XNU_PT_ATTACHEXC:
		{
			// Use darlingserver mechanisms to set a tracing task (->disallow anyone else to attach)
			ret = dserver_rpc_set_tracer(pid, getpid());
			if (ret < 0) {
				__simple_printf("dserver_rpc_set_tracer failed internally: %d", ret);
				__simple_abort();
			}
			ret = errno_linux_to_bsd(-ret);

			// darlingserver refused the attach (e.g. EBUSY after
			// PT_DENY_ATTACH, when the target's policy enforces it):
			// leave the target alone
			if (ret != 0) {
				cmd = "PT_ATTACHEXC";
				break;
			}

			tracee_add(pid);
			sys_kill(pid, SIGSTOP, 1);

			ret = dserver_rpc_ptrace_sigexc(pid, true);
			if (ret < 0) {
				__simple_printf("dserver_rpc_ptrace_sigexc failed internally: %d", ret);
				__simple_abort();
			}
			ret = errno_linux_to_bsd(-ret);

			cmd = "PT_ATTACHEXC";
			break;
		}
		case XNU_PT_ATTACH:
		{
			ret = dserver_rpc_set_tracer(pid, getpid());
			if (ret < 0) {
				__simple_printf("dserver_rpc_set_tracer failed internally: %d", ret);
				__simple_abort();
			}
			ret = errno_linux_to_bsd(-ret);
			if (ret == 0) {
				// the signal-based variant of PT_ATTACHEXC: the target is stopped,
				// and PT_CONTINUE resumes it
				tracee_add(pid);
				sys_kill(pid, SIGSTOP, 1);
			}
			cmd = "PT_ATTACH"; break;
		}
		case XNU_PT_DETACH:
		{
			// Tell the tracee to restore original application signal handlers.
			//linux_sigqueue(pid, SIGNAL_SIGEXC_TOGGLE, SIGRT_MAGIC_DISABLE_SIGEXC);

			//ret = 0; //LINUX_SYSCALL(__NR_ptrace, LINUX_PTRACE_DETACH, pid, addr, data);

			ret = dserver_rpc_ptrace_sigexc(pid, false);
			if (ret < 0) {
				__simple_printf("dserver_rpc_ptrace_sigexc failed internally: %d", ret);
				__simple_abort();
			}
			ret = errno_linux_to_bsd(-ret);

			// the caller stops being the target's tracer, and with it the
			// effective parent for wait purposes
			tracee_remove(pid);
			dserver_rpc_set_tracer(pid, 0);

			cmd = "PT_DETACH"; break;
		}
		case XNU_PT_SIGEXC:
		{
			__simple_kprintf("sigexc: self via ptrace\n");

			ret = dserver_rpc_ptrace_sigexc(getpid(), true);
			if (ret < 0) {
				__simple_printf("dserver_rpc_ptrace_sigexc failed internally: %d", ret);
				__simple_abort();
			}
			ret = errno_linux_to_bsd(-ret);

			// return ret;
			cmd = "PT_SIGEXC"; break;
		}
		case XNU_PT_CONTINUE:
		{
			ret = sys_kill(pid, SIGCONT, 1);

			/*
			int signal = 0;
			if (data != -1)
				signal = signum_bsd_to_linux(data);
			ret = LINUX_SYSCALL(__NR_ptrace, LINUX_PTRACE_CONT, pid, NULL, signal);
			
			if (ret < 0)
				ret = errno_linux_to_bsd(ret);
			*/
			
			// return ret;
			cmd = "PT_CONTINUE"; break;
		}
		case XNU_PT_THUPDATE:
		{
			// Convert thread_t to process ID
			int tid = -1;
			ret = dserver_rpc_tid_for_thread(addr, &tid);
			if (ret < 0) {
				__simple_printf("dserver_rpc_tid_for_thread failed internally: %d", ret);
				__simple_abort();
			} else if (ret > 0) {
				ret = errno_linux_to_bsd(-ret);
				return ret;
			}

			ret = dserver_rpc_ptrace_thupdate(tid, data);
			if (ret < 0) {
				__simple_printf("dserver_rpc_ptrace_thupdate failed internally: %d", ret);
				__simple_abort();
			}
			ret = errno_linux_to_bsd(-ret);

			// return ret;
			cmd = "PT_THUPDATE"; break;
		}
		case XNU_PT_DENY_ATTACH:
		{
			// Recorded by darlingserver, which refuses later attach
			// requests when the process's code signing policy enforces
			// it. The request itself succeeds; callers treat any other
			// result as a failure.
			ret = dserver_rpc_cs_deny_attach();
			if (ret < 0) {
				__simple_printf("dserver_rpc_cs_deny_attach failed internally: %d", ret);
				__simple_abort();
			}
			ret = 0;
			cmd = "PT_DENY_ATTACH"; break;
		}
	}

	if (cmd != NULL)
		__simple_kprintf("ptrace() req=%s, ret=%d\n", cmd, ret);
	else
		__simple_kprintf("ptrace() req=%d\n", request);

	return ret;
}
