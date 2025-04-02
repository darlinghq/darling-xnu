#include <emulation/xnu_syscall/bsd/impl/signal/kill.h>

#include <emulation/conversion/methods/errno.h>
#include <emulation/conversion/methods/signal.h>
#include <emulation/linux_api/linux_syscall.h>

extern int getpid(void);

long sys_kill(int pid, int signum, int posix)
{
	int ret, linux_signum;

	// If we're stopping a process we're debugging, do an emulated SIGSTOP
	// so that the tracee has a chance to talk to us before stopping.
	/*if (signum == SIGSTOP && pid > 0)
	{
		int tracer = lkm_call(NR_get_tracer, (void*)(long)pid);
		if (tracer == getpid())
		{
			linux_sigqueue(pid, SIGNAL_SIGEXC_THUPDATE, -LINUX_SIGSTOP);
			return 0;
		}
	}*/

	linux_signum = signum_bsd_to_linux(signum);
	if (signum && !linux_signum)
		return -BSD_EINVAL;

	ret = LINUX_SYSCALL(__NR_kill, pid, linux_signum);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

