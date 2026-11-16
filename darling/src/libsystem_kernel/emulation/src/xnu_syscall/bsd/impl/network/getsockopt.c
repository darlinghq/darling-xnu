#include <darling/emulation/xnu_syscall/bsd/impl/network/getsockopt.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/network/getsockopt.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/duct.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/nosigpipe.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/sockopt_extra.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/gettimeofday.h>

extern void* memcpy(void*, const void*, __SIZE_TYPE__);

long sys_getsockopt(int fd, int level, int optname, void* optval, int* optlen)
{
	int ret, linux_level, linux_optname;

	if (level == BSD_SOL_SOCKET && optname == BSD_SO_NOSIGPIPE)
	{
		int type, value, len = sizeof(type);

		ret = sys_getsockopt(fd, BSD_SOL_SOCKET, BSD_SO_TYPE, &type, &len);
		if (ret < 0)
			return ret;
		if (!optval || !optlen)
			return -EFAULT;

		// A shorter buffer gets the leading bytes, as for any int option.
		value = nosigpipe_get(fd);
		if (*optlen > (int)sizeof(value))
			*optlen = sizeof(value);
		else if (*optlen < 0)
			return -EINVAL;
		memcpy(optval, &value, *optlen);
		return 0;
	}

	// XNU keeps these options itself; Linux has no equivalent with the same numbering or scale.
	if (level == BSD_SOL_SOCKET && (optname == BSD_SO_TRAFFIC_CLASS ||
		optname == BSD_SO_RECV_TRAFFIC_CLASS || optname == BSD_SO_NET_SERVICE_TYPE))
		return sockopt_extra_get_tc(fd, optname, optval, optlen);
	if ((level == BSD_SOL_SOCKET && optname == BSD_SO_RECV_ANYIF) ||
		(level == 0 && (optname == BSD_IP_RECVIF || optname == BSD_IP_RECVDSTADDR_OPT)))
		return sockopt_extra_get_flag(fd, level, optname, optval, optlen);

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
			else if ((optname == BSD_SO_RCVTIMEO || optname == BSD_SO_SNDTIMEO)
					&& optval && optlen && *optlen >= (int)sizeof(struct linux_timeval))
			{
				// Linux returns a timeval with a long tv_usec; Darwin's is
				// 32 bits followed by padding. The layouts only differ on
				// big-endian (PPC), but convert both timeouts the same way.
				struct linux_timeval ltv = *((struct linux_timeval*) optval);
				struct bsd_timeval* btv = (struct bsd_timeval*) optval;
				btv->tv_sec = ltv.tv_sec;
				btv->tv_usec = ltv.tv_usec;
			}
		}
	}

	return ret;
}
