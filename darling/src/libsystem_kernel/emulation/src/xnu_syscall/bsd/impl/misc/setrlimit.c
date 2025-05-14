#include <darling/emulation/xnu_syscall/bsd/impl/misc/setrlimit.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_setrlimit(unsigned int which, const struct xnu_rlimit* rlp)
{
	int ret;
	struct xnu_rlimit lim = { rlp->rlim_cur, rlp->rlim_max };

	which = rlimit_bsd_to_linux(which);
	if (which == -1)
		return -EINVAL;

	if (which == LINUX_RLIMIT_NOFILE)
	{
		lim.rlim_cur++;
		lim.rlim_max++;
	}

	ret = LINUX_SYSCALL(__NR_prlimit64, 0, which, &lim, 0);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
