#include <darling/emulation/xnu_syscall/bsd/impl/misc/getrlimit.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/nofile_limit.h>

long sys_getrlimit(unsigned int which, struct xnu_rlimit* rlp)
{
	int ret;

	int lwhich = (int)rlimit_bsd_to_linux(which);
	if (lwhich < 0)
		return -EINVAL;

	// Linux accepts a NULL old-limit pointer; Darwin fails the copyout.
	if (rlp == NULL)
		return -EFAULT;

	// Kept apart from the Linux limit, see nofile_limit.c. It is never
	// reported above OPEN_MAX: Darwin programs size descriptor loops and
	// tables by it.
	if (lwhich == LINUX_RLIMIT_NOFILE)
		return nofile_limit_get(rlp);

	ret = LINUX_SYSCALL(__NR_prlimit64, 0, (unsigned int)lwhich, 0, rlp);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	rlp->rlim_cur = rlim_linux_to_bsd(rlp->rlim_cur);
	rlp->rlim_max = rlim_linux_to_bsd(rlp->rlim_max);

	return ret;
}

unsigned int rlimit_bsd_to_linux(unsigned int which)
{
	which &= ~BSD_RLIMIT_POSIX;

	switch (which)
	{
		case BSD_RLIMIT_CPU:
			return LINUX_RLIMIT_CPU;
		case BSD_RLIMIT_FSIZE:
			return LINUX_RLIMIT_FSIZE;
		case BSD_RLIMIT_DATA:
			return LINUX_RLIMIT_DATA;
		case BSD_RLIMIT_STACK:
			return LINUX_RLIMIT_STACK;
		case BSD_RLIMIT_CORE:
			return LINUX_RLIMIT_CORE;
		case BSD_RLIMIT_AS:
			return LINUX_RLIMIT_AS;
		case BSD_RLIMIT_MEMLOCK:
			return LINUX_RLIMIT_MEMLOCK;
		case BSD_RLIMIT_NPROC:
			return LINUX_RLIMIT_NPROC;
		case BSD_RLIMIT_NOFILE:
			return LINUX_RLIMIT_NOFILE;
		default:
			return -1;
	}
}
