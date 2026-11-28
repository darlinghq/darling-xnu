#include <darling/emulation/xnu_syscall/bsd/helper/network/sockopt_extra.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern void* memcpy(void* dest, const void* src, __SIZE_TYPE__ n);
extern void* memset(void* dest, int c, __SIZE_TYPE__ n);

#define EXTRA_FDS 8192

#define L_SOL_SOCKET 1
#define L_SO_DOMAIN 39
#define L_AF_INET 2
#define L_AF_INET6 10
#define L_IP_PKTINFO 8
#define L_SYS_GETSOCKOPT 15
#define L_SYS_SETSOCKOPT 14

static long l_getsockopt(int fd, int level, int name, void* value, unsigned int* len)
{
#ifdef __NR_socketcall
	return LINUX_SYSCALL(__NR_socketcall, L_SYS_GETSOCKOPT,
			((long[6]) { fd, level, name, (long)value, (long)len, 0 }));
#else
	return LINUX_SYSCALL(__NR_getsockopt, fd, level, name, value, len);
#endif
}

static long l_setsockopt(int fd, int level, int name, const void* value, unsigned int len)
{
#ifdef __NR_socketcall
	return LINUX_SYSCALL(__NR_socketcall, L_SYS_SETSOCKOPT,
			((long[6]) { fd, level, name, (long)value, len, 0 }));
#else
	return LINUX_SYSCALL(__NR_setsockopt, fd, level, name, value, len);
#endif
}

static int l_domain(int fd)
{
	int domain = 0;
	unsigned int len = sizeof(domain);
	return l_getsockopt(fd, L_SOL_SOCKET, L_SO_DOMAIN, &domain, &len) < 0 ? -1 : domain;
}

// Traffic class options (<sys/socket.h>): SO_TRAFFIC_CLASS takes one of the SO_TC_* classes 0, 100, ..., 900, or
// SO_TC_NET_SERVICE_OFFSET (10000) plus a network service type 0..8 (which sets the class that service type maps
// to); SO_NET_SERVICE_TYPE takes the service type itself, 0..8. Anything else is EINVAL. SO_RECV_TRAFFIC_CLASS is
// a flag. Linux has no option with the same scale, so the values are kept per descriptor and read back from there.
// The class for each service type was observed on macOS 26.7.3 (black-box, tier 2); the documented names are
// NET_SERVICE_TYPE_BE 0, BK 1, SIG 2, VI 3, VO 4, RV 5, AV 6, OAM 7, RD 8.
#define SO_TC_NET_SERVICE_OFFSET 10000
#define NET_SERVICE_TYPE_RD 8
static short tc_class[EXTRA_FDS];
static signed char tc_svc[EXTRA_FDS];
static unsigned char tc_recv[EXTRA_FDS / 8];
static unsigned char tc_svc_set[EXTRA_FDS / 8];
static const short tc_by_svc[NET_SERVICE_TYPE_RD + 1] = { 0, 200, 700, 700, 800, 600, 500, 400, 300 };

static int tc_sock(int fd)
{
	return l_domain(fd) < 0 ? -ENOTSOCK : 0;
}

long sockopt_extra_set_tc(int fd, int optname, const void* optval, int optlen)
{
	int r = tc_sock(fd);
	if (r < 0)
		return r;
	if (optval == NULL || optlen < (int)sizeof(int))
		return -EINVAL;
	int v = *(const int*) optval;
	if (fd < 0 || fd >= EXTRA_FDS)
		return 0;
	if (optname == BSD_SO_RECV_TRAFFIC_CLASS)
	{
		if (v) tc_recv[fd / 8] |= (unsigned char)(1 << (fd % 8));
		else tc_recv[fd / 8] &= (unsigned char)~(1 << (fd % 8));
		return 0;
	}

	int svc = -1;
	if (optname == BSD_SO_NET_SERVICE_TYPE)
		svc = v;
	else if (v >= SO_TC_NET_SERVICE_OFFSET)
		svc = v - SO_TC_NET_SERVICE_OFFSET;
	if (svc >= 0 || optname == BSD_SO_NET_SERVICE_TYPE)
	{
		if (svc < 0 || svc > NET_SERVICE_TYPE_RD)
			return -EINVAL;
		tc_class[fd] = tc_by_svc[svc];
		tc_svc[fd] = (signed char)svc;
		tc_svc_set[fd / 8] |= (unsigned char)(1 << (fd % 8));
		return 0;
	}

	// SO_TC_BE (0) .. SO_TC_CTL (900) in steps of 100; observed on macOS 26.7.3: 250, -1 and 1000 are EINVAL
	if (v < 0 || v > 900 || v % 100 != 0)
		return -EINVAL;
	tc_class[fd] = (short) v;
	tc_svc_set[fd / 8] &= (unsigned char)~(1 << (fd % 8));
	return 0;
}

static long tc_option_value(long v, void* optval, int* optlen)
{
	if (optlen == NULL)
		return -EFAULT;
	if (optval == NULL)
	{
		*optlen = 0;
		return 0;
	}
	int n = *optlen < (int)sizeof(v) ? *optlen : (int)sizeof(v);
	if (n < 0)
		return -EINVAL;
	for (int i = 0; i < n; i++)
		((char*)optval)[i] = ((char*)&v)[i];
	*optlen = n;
	return 0;
}

long sockopt_extra_get_tc(int fd, int optname, void* optval, int* optlen)
{
	int r = tc_sock(fd);
	if (r < 0)
		return r;
	int v = 0;
	if (fd >= 0 && fd < EXTRA_FDS)
	{
		// QUIRK: SO_RECV_TRAFFIC_CLASS reads back as 0x4000 (not 1) when set. Undocumented; observed on macOS 26.7.3
		// (tier 2). Test: tests/src/libsystem_kernel_socket_options.c (recv traffic class).
		if (optname == BSD_SO_RECV_TRAFFIC_CLASS)
			v = (tc_recv[fd / 8] & (1 << (fd % 8))) ? 0x4000 : 0;
		else if (optname == BSD_SO_NET_SERVICE_TYPE)
			v = (tc_svc_set[fd / 8] & (1 << (fd % 8))) ? tc_svc[fd] : 0;
		else
			v = tc_class[fd];
	}
	return tc_option_value(v, optval, optlen);
}

static unsigned char anyif_flags[EXTRA_FDS / 8], recvif_flags[EXTRA_FDS / 8], recvdst_flags[EXTRA_FDS / 8],
	pktinfo_user[EXTRA_FDS / 8];

#define BIT(a, fd) ((fd) >= 0 && (fd) < EXTRA_FDS && (((a)[(fd) / 8] & (1 << ((fd) % 8))) != 0))

static void setbit_(unsigned char* a, int fd, int on)
{
	if (fd < 0 || fd >= EXTRA_FDS)
		return;
	if (on) a[fd / 8] |= (unsigned char)(1 << (fd % 8));
	else a[fd / 8] &= (unsigned char)~(1 << (fd % 8));
}

void sockopt_extra_note_pktinfo(int fd, int on) { setbit_(pktinfo_user, fd, on); }
int sockopt_extra_recvif(int fd) { return BIT(recvif_flags, fd); }
int sockopt_extra_recvdst(int fd) { return BIT(recvdst_flags, fd); }
int sockopt_extra_pktinfo_user(int fd) { return BIT(pktinfo_user, fd); }

long sockopt_extra_set_flag(int fd, int level, int optname, const void* optval, int optlen)
{
	int domain = l_domain(fd);
	if (domain < 0)
		return -ENOTSOCK;
	if (optval == NULL || optlen < (int)sizeof(int))
		return -EINVAL;
	int on = *(const int*)optval != 0;
	if (level == 0 && (optname == BSD_IP_RECVIF || optname == BSD_IP_RECVDSTADDR_OPT))
	{
		if (domain != L_AF_INET && domain != L_AF_INET6)
			return -EINVAL;
		// Linux's IP_PKTINFO carries both the interface index and the destination address. Keep it enabled
		// while any of the three options is on.
		int other = optname == BSD_IP_RECVIF ? BIT(recvdst_flags, fd) : BIT(recvif_flags, fd);
		int v = on || other || BIT(pktinfo_user, fd);
		long r = l_setsockopt(fd, 0, L_IP_PKTINFO, &v, sizeof(v));
		if (r < 0)
			return errno_linux_to_bsd(r);
		setbit_(optname == BSD_IP_RECVIF ? recvif_flags : recvdst_flags, fd, on);
		return 0;
	}
	setbit_(anyif_flags, fd, on);
	return 0;
}

long sockopt_extra_get_flag(int fd, int level, int optname, void* optval, int* optlen)
{
	if (l_domain(fd) < 0)
		return -ENOTSOCK;
	int v;
	if (level == 0 && optname == BSD_IP_RECVIF)
		v = sockopt_extra_recvif(fd);
	else if (level == 0 && optname == BSD_IP_RECVDSTADDR_OPT)
		v = sockopt_extra_recvdst(fd);
	else
		v = BIT(anyif_flags, fd);
	return tc_option_value(v, optval, optlen);
}

void sockopt_extra_closed(int fd)
{
	setbit_(anyif_flags, fd, 0);
	setbit_(recvif_flags, fd, 0);
	setbit_(recvdst_flags, fd, 0);
	setbit_(pktinfo_user, fd, 0);
	if (fd >= 0 && fd < EXTRA_FDS)
	{
		tc_class[fd] = 0;
		tc_recv[fd / 8] &= (unsigned char)~(1 << (fd % 8));
		tc_svc_set[fd / 8] &= (unsigned char)~(1 << (fd % 8));
	}
}
