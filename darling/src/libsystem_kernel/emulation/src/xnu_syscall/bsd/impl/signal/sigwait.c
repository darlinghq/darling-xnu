#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigwait.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigaction.h>

static long sys_sigwait_common(sigset_t* set, int* sig, bool cancelable);

long sys_sigwait(sigset_t* set, int* sig)
{
	CANCELATION_POINT();
	return sys_sigwait_common(set, sig, true);
}

long sys_sigwait_nocancel(sigset_t* set, int* sig)
{
	return sys_sigwait_common(set, sig, false);
}

static long sys_sigwait_common(sigset_t* set, int* sig, bool cancelable)
{
	linux_sigset_t set_linux;
	int ret;
	struct linux_siginfo si;

	sigset_bsd_to_linux(set, &set_linux);

	ret = LINUX_SYSCALL_C(cancelable, __NR_rt_sigtimedwait, &set_linux, &si, NULL, 8);

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else if (sig)
		*sig = signum_linux_to_bsd(si.si_signo);

	return ret;
}
