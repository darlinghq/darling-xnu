#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigpending.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_sigpending(sigset_t* set)
{
	linux_sigset_t set_linux;
	int ret;

	// sigpending(2) reports no errors: XNU stores the set only when one is given
	if (set == NULL)
		return 0;

	ret = LINUX_SYSCALL(__NR_rt_sigpending, &set_linux, sizeof(linux_sigset_t));

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else
		sigset_linux_to_bsd(&set_linux, set);

	return ret;
}
