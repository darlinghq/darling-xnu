#include <darling/emulation/xnu_syscall/bsd/impl/network/setsockopt.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/xnu_syscall/bsd/impl/network/getsockopt.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/network/getsockopt.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/gettimeofday.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/duct.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/nosigpipe.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/sockopt_extra.h>

#define LINGER_TICKS_PER_SEC	100 // Is this the right number of ticks per sec?

// Fails like any option on a descriptor that is not a socket.
static long check_socket(int fd)
{
	int type;
	int len = sizeof(type);

	return sys_getsockopt(fd, BSD_SOL_SOCKET, BSD_SO_TYPE, &type, &len);
}

long sys_setsockopt(int fd, int level, int optname, const void* optval, int optlen)
{
	int ret, linux_level, linux_optname;
	char buf[100];

	if (level == BSD_SOL_SOCKET && optname == BSD_SO_NOSIGPIPE)
	{
		ret = check_socket(fd);
		if (ret < 0)
			return ret;
		if (optlen < (int)sizeof(int))
			return -EINVAL;
		if (!optval)
			return -EFAULT;
		return nosigpipe_set(fd, *(const int*) optval != 0);
	}

	// XNU keeps these options itself; Linux has no equivalent with the same numbering or scale.
	if (level == BSD_SOL_SOCKET && (optname == BSD_SO_TRAFFIC_CLASS ||
		optname == BSD_SO_RECV_TRAFFIC_CLASS || optname == BSD_SO_NET_SERVICE_TYPE))
		return sockopt_extra_set_tc(fd, optname, optval, optlen);

	// The protocol-independent multicast API (RFC 3678: MCAST_JOIN_GROUP 80 .. MCAST_UNBLOCK_SOURCE 85 at
	// IPPROTO_IP or IPPROTO_IPV6). Linux has the same operations at 42..47 with the same payload layout, but
	// Darwin packs the sockaddr_storage at 4/132 and Linux at 8/136, and the family has no sa_len byte.
	if ((level == 0 || level == 41) && optname >= 80 && optname <= 85)
	{
		static const int lname[6] = { 42, 45, 46, 47, 43, 44 };
		int nsa = optname >= 82 ? 2 : 1;
		int bsd_len = 4 + 128 * nsa, linux_len = 8 + 128 * nsa;
		unsigned char gr[8 + 128 * 2];
		if (optval == NULL || optlen < bsd_len)
			return -EINVAL;
		const unsigned char* in = optval;
		for (int i = 0; i < linux_len; i++) gr[i] = 0;
		for (int i = 0; i < 4; i++) gr[i] = in[i];
		for (int k = 0; k < nsa; k++) {
			const unsigned char* bsa = in + 4 + 128 * k;
			unsigned char* sa = gr + 8 + 128 * k;
			for (int i = 0; i < 128; i++) sa[i] = bsa[i];
			int fam = bsa[1] == 30 ? 10 : bsa[1]; // AF_INET6 30 -> 10; AF_INET 2 on both
			sa[0] = (unsigned char) fam; sa[1] = (unsigned char) (fam >> 8);
		}
#ifdef __NR_socketcall
		ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_SETSOCKOPT,
				((long[6]) { fd, level, lname[optname - 80], gr, linux_len, 0 }));
#else
		ret = LINUX_SYSCALL(__NR_setsockopt, fd, level, lname[optname - 80], gr, linux_len);
#endif
		return ret < 0 ? errno_linux_to_bsd(ret) : 0;
	}
	if ((level == BSD_SOL_SOCKET && optname == BSD_SO_RECV_ANYIF) ||
		(level == 0 && (optname == BSD_IP_RECVIF || optname == BSD_IP_RECVDSTADDR_OPT)))
		return sockopt_extra_set_flag(fd, level, optname, optval, optlen);

	// IP_MULTICAST_IFINDEX takes an int; Linux's IP_MULTICAST_IF takes a struct ip_mreqn with the index last.
	if (level == 0 && optname == 66)
	{
		if (optval == NULL || optlen < (int)sizeof(int))
			return -EINVAL;
		struct { unsigned int multiaddr, address; int ifindex; } mr = { 0, 0, *(const int*) optval };
#ifdef __NR_socketcall
		ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_SETSOCKOPT,
				((long[6]) { fd, 0, LINUX_IP_MULTICAST_IF, &mr, sizeof(mr), 0 }));
#else
		ret = LINUX_SYSCALL(__NR_setsockopt, fd, 0, LINUX_IP_MULTICAST_IF, &mr, sizeof(mr));
#endif
		return ret < 0 ? errno_linux_to_bsd(ret) : 0;
	}
	if (level == 0 && optname == BSD_IP_RECVPKTINFO && optval != NULL && optlen >= (int)sizeof(int))
	{
		// Keep Linux's IP_PKTINFO on while IP_RECVIF/IP_RECVDSTADDR need it, even if the program turned it off.
		int on = *(const int*) optval != 0;
		int v = on || sockopt_extra_recvif(fd) || sockopt_extra_recvdst(fd);
#ifdef __NR_socketcall
		ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_SETSOCKOPT,
				((long[6]) { fd, 0, LINUX_IP_PKTINFO, &v, sizeof(v), 0 }));
#else
		ret = LINUX_SYSCALL(__NR_setsockopt, fd, 0, LINUX_IP_PKTINFO, &v, sizeof(v));
#endif
		if (ret < 0)
			return errno_linux_to_bsd(ret);
		sockopt_extra_note_pktinfo(fd, on);
		return 0;
	}

	linux_level = level;
	linux_optname = optname;
	
	ret = sockopt_bsd_to_linux(&linux_level, &linux_optname,
			(void**) &optval, buf);
	
	if (ret != 0 || !linux_optname)
		return ret;

	if (level == BSD_SOL_SOCKET)
	{
		if (optname == BSD_SO_RCVTIMEO || optname == BSD_SO_SNDTIMEO)
		{
			if (optlen < (int)sizeof(struct bsd_timeval))
				return -EINVAL;
			if (!optval)
				return -EFAULT;
			const struct bsd_timeval* tv = (const struct bsd_timeval*) optval;
			if ((long)tv->tv_sec < 0 || tv->tv_usec < 0 || tv->tv_usec >= 1000000)
				return -EDOM;
			struct linux_timeval* ltv = (struct linux_timeval*) __builtin_alloca(sizeof(struct linux_timeval));

			ltv->tv_sec = tv->tv_sec;
			ltv->tv_usec = tv->tv_usec;

			optval = ltv;
			optlen = sizeof(*ltv);
		}
	}

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_SETSOCKOPT,
			((long[6]) { fd, linux_level, linux_optname, optval, optlen, 0 }));
#else
	ret = LINUX_SYSCALL(__NR_setsockopt, fd, linux_level, linux_optname,
			optval, optlen);
#endif

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
