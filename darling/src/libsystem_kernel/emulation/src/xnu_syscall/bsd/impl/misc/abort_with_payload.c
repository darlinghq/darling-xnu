#include <darling/emulation/xnu_syscall/bsd/impl/misc/abort_with_payload.h>

#include <sys/signal.h>

#include <darling/emulation/xnu_syscall/bsd/impl/signal/kill.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_abort_with_payload(unsigned int reason_namespace, unsigned long long reason_code, void *payload, unsigned int payload_size, const char *reason_string, unsigned long long reason_flags)
{
	__simple_printf("abort_with_payload: reason: %s; code: %lu\n",
	                reason_string ? reason_string : "(null)", reason_code);
	// First raise SIGABRT for the current thread so a coredump / signal
	// handler can run. Then ensure the process actually terminates — caller
	// (dyld's abort_with_payload_wrapper) treats this as __noreturn__ and
	// emits `brk #1` if we ever return.
	sys_kill(0, SIGABRT, 1);
	LINUX_SYSCALL1(__NR_exit_group, 128 + SIGABRT);
	__builtin_unreachable();
}

long sys_terminate_with_payload(int pid, unsigned int reason_namespace, unsigned long long reason_code, void *payload, unsigned int payload_size, const char *reason_string, unsigned long long reason_flags)
{
	__simple_printf("terminate_with_payload: pid=%d reason: %s; code: %lu\n",
	                pid, reason_string ? reason_string : "(null)", reason_code);
	// On Darwin this targets the given pid; we approximate by exiting if it
	// refers to us (pid<=0 means current process group / -1 means self in
	// many call sites). For other pids we send SIGKILL.
	if (pid <= 0) {
		LINUX_SYSCALL1(__NR_exit_group, 128 + SIGKILL);
		__builtin_unreachable();
	}
	sys_kill(pid, SIGKILL, 1);
	return 0;
}
