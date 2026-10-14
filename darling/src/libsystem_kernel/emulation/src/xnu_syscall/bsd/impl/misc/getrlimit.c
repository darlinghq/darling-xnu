#include <darling/emulation/xnu_syscall/bsd/impl/misc/getrlimit.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define DARWIN_OPEN_MAX 10240

long sys_getrlimit(unsigned int which, struct xnu_rlimit* rlp)
{
	int ret;

	int lwhich = (int)rlimit_bsd_to_linux(which);
	if (lwhich < 0)
		return -EINVAL;

	ret = LINUX_SYSCALL(__NR_prlimit64, 0, (unsigned int)lwhich, 0, rlp);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	// BSD_RLIMIT_POSIX (0x1000) is the SUSv3 conformance bit flag in macOS libc.
	// In macOS, RLIMIT_NOFILE is always capped at OPEN_MAX (10240) and never returns
	// unbounded Linux host limits or RLIM_INFINITY.
	else if (rlp != NULL && (which & ~BSD_RLIMIT_POSIX) == BSD_RLIMIT_NOFILE)
	{
		if (rlp->rlim_cur > DARWIN_OPEN_MAX)
			rlp->rlim_cur = DARWIN_OPEN_MAX;
		if (rlp->rlim_max > DARWIN_OPEN_MAX)
			rlp->rlim_max = DARWIN_OPEN_MAX;
	}

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
