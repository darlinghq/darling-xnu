#include <darling/emulation/legacy_path/signal/sigwait.h>

#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/signal/duct_signals.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/bsdthread/cancelable.h>
#include <darling/emulation/legacy_path/signal/sigaction.h>

long sys_sigwait(sigset_t* set, int* sig)
{
	CANCELATION_POINT();
	return sys_sigwait_nocancel(set, sig);
}

long sys_sigwait_nocancel(sigset_t* set, int* sig)
{
	linux_sigset_t set_linux;
	int ret;
	struct linux_siginfo si;

	sigset_bsd_to_linux(set, &set_linux);

	ret = LINUX_SYSCALL(__NR_rt_sigtimedwait, &set_linux, &si, NULL, 8);

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else if (sig)
		*sig = signum_linux_to_bsd(si.si_signo);

	return ret;
}

