#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigpending.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_sigpending(sigset_t* set)
{
	linux_sigset_t set_linux;
	int ret;

	// XNU copies the pending set out to the caller and fails with EFAULT when it cannot be written
	if (set == NULL)
		return -EFAULT;

	ret = LINUX_SYSCALL(__NR_rt_sigpending, &set_linux, sizeof(linux_sigset_t));

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else
		sigset_linux_to_bsd(&set_linux, set);

	return ret;
}
