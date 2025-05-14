#include <darling/emulation/xnu_syscall/bsd/impl/network/getsockopt.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/network/getsockopt.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/duct.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/gettimeofday.h>

long sys_getsockopt(int fd, int level, int optname, void* optval, int* optlen)
{
	int ret, linux_level, linux_optname;
	
	linux_level = level;
	linux_optname = optname;
	
	ret = sockopt_bsd_to_linux(&linux_level, &linux_optname, NULL, optval);
	
	if (ret != 0 || !linux_optname)
		return ret;

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_GETSOCKOPT,
			((long[6]) { fd, linux_level, linux_optname, optval, optlen, 0 }));
#else
	ret = LINUX_SYSCALL(__NR_getsockopt, fd, linux_level, linux_optname,
			optval, optlen);
#endif

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else
	{
		if (level == BSD_SOL_SOCKET)
		{
			if (optname == BSD_SO_LINGER_TICKS)
			{
				struct xnu_linger* l = (struct xnu_linger*) optval;
				l->l_linger *= LINGER_TICKS_PER_SEC;
			}
			else if (optname == BSD_SO_ERROR)
			{
				int* err = (int*) optval;
				if (err && *err)
					*err = errno_linux_to_bsd(*err);
			}
			else if (optname == BSD_SO_RCVTIMEO)
			{
				// This only makes a difference on big-endian (PPC)
				struct linux_timeval ltv = *((struct linux_timeval*) optval);
				struct bsd_timeval* btv = (struct bsd_timeval*) optval;
				btv->tv_sec = ltv.tv_sec;
				btv->tv_usec = ltv.tv_usec;
			}
		}
	}

	return ret;
}
