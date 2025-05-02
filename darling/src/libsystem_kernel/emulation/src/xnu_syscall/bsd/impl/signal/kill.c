#include <darling/emulation/xnu_syscall/bsd/impl/signal/kill.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/linux_premigration/signal/sigexc.h>

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
		return -EINVAL;

	ret = LINUX_SYSCALL(__NR_kill, pid, linux_signum);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
