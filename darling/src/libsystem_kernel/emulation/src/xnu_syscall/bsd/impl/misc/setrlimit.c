#include <darling/emulation/xnu_syscall/bsd/impl/misc/setrlimit.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/nofile_limit.h>

long sys_setrlimit(unsigned int which, const struct xnu_rlimit* rlp)
{
	int ret;
	struct xnu_rlimit lim;

	if (rlp == NULL)
		return -EFAULT;

	lim.rlim_cur = rlim_bsd_to_linux(rlp->rlim_cur);
	lim.rlim_max = rlim_bsd_to_linux(rlp->rlim_max);

	which = rlimit_bsd_to_linux(which);
	if (which == -1)
		return -EINVAL;

	// Checked on the Darwin values: the infinity mapping can make a soft
	// limit above the hard limit look valid.
	if (rlp->rlim_cur > rlp->rlim_max)
		return -EINVAL;

	// Kept apart from the Linux limit, see nofile_limit.c.
	if (which == LINUX_RLIMIT_NOFILE)
		return nofile_limit_set(rlp);

	ret = LINUX_SYSCALL(__NR_prlimit64, 0, which, &lim, 0);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
