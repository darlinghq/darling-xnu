#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigsuspend.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

long sys_sigsuspend(sigset_t set)
{
	CANCELATION_POINT();
	return sys_sigsuspend_nocancel(set);
}

long sys_sigsuspend_nocancel(sigset_t set)
{
	linux_sigset_t set_linux;
	int ret;

	sigset_bsd_to_linux(&set, &set_linux);

	ret = LINUX_SYSCALL(__NR_rt_sigsuspend, &set_linux, 8);

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
