#include <darling/emulation/xnu_syscall/bsd/impl/network/connect.h>

#include <sys/socket.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/duct.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

extern void *memcpy(void *dest, const void *src, __SIZE_TYPE__ n);
extern __SIZE_TYPE__ strlen(const char* src);
extern char* strcpy(char* dest, const char* src);
extern char *strncpy(char *dest, const char *src, __SIZE_TYPE__ n);

// Must be included after strncpy
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

// libkqueue parks EVFILT_READ/WRITE knotes on a stream socket that was never connected. A connect attempt ends
// that state whatever its result (even a connect still in progress), so let libkqueue re-arm its knotes.
__attribute__((weak))
__attribute__((visibility("default")))
void kqueue_socket_unpark(int fd)
{
}

static long sys_connect_common(int fd, const void* name, int socklen, bool cancelable);

#include <darling/emulation/xnu_syscall/bsd/impl/network/getsockopt.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/getpeername.h>

// QUIRK: on a non-blocking stream socket whose connection attempt has
// completed, a further connect() fails with EISCONN. Linux instead reports
// success once, because it treats the call as confirming the pending attempt.
// Evidence: black-box observation on macOS 26.7.3 (tier 2): the repeated call
// returns EALREADY while the attempt is pending and EISCONN afterwards, never 0;
// connect(2) documents EISCONN for an already connected socket.
// Test: tests/src/socket_nonblocking_connect.c (check_repeat_done and the
// repeated connect right after the connect started).
static int stream_already_connected(int fd)
{
	int type = 0, typelen = sizeof(type);
	long flags = LINUX_SYSCALL(__NR_fcntl, fd, 3 /*F_GETFL*/, 0);
	char peer[128];
	int peerlen = sizeof(peer);

	if (flags < 0 || !(flags & 0x800 /*O_NONBLOCK*/))
		return 0;
	if (sys_getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &typelen) < 0 || type != SOCK_STREAM)
		return 0;
	return sys_getpeername(fd, peer, &peerlen) == 0;
}

long sys_connect(int fd, const void* name, int socklen)
{
	CANCELATION_POINT();
	return sys_connect_common(fd, name, socklen, true);
}

long sys_connect_nocancel(int fd, const void* name, int socklen)
{
	return sys_connect_common(fd, name, socklen, false);
}

static long sys_connect_common(int fd, const void* name, int socklen, bool cancelable)
{
	int ret;
	struct sockaddr_fixup* fixed;

	if (socklen > 512)
		return -EINVAL;

	fixed = __builtin_alloca(sockaddr_fixup_size_from_bsd(name, socklen));
	ret = socklen = sockaddr_fixup_from_bsd(fixed, name, socklen);
	if (ret < 0)
		return ret;

	if (((const struct sockaddr*) name)->sa_family != AF_UNSPEC && stream_already_connected(fd))
		return -EISCONN;

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL_C(cancelable, __NR_socketcall, LINUX_SYS_CONNECT, ((long[6]) { fd, fixed, socklen }));
#else
	ret = LINUX_SYSCALL_C(cancelable, __NR_connect, fd, fixed, socklen);
#endif

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	kqueue_socket_unpark(fd);

	return ret;
}

#include <darling/emulation/xnu_syscall/bsd/impl/network/bind.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/writev.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/shutdown.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/getsockopt.h>

struct darling_sa_endpoints {
	unsigned int            sae_srcif;
	const struct sockaddr   *sae_srcaddr;
	socklen_t               sae_srcaddrlen;
	const struct sockaddr   *sae_dstaddr;
	socklen_t               sae_dstaddrlen;
};

long sys_connectx(int fd, const void* endpoints_arg, unsigned int associd, unsigned int flags, const void* iov, unsigned int iovcnt, void* len, void* connid)
{
	CANCELATION_POINT();

	if (!endpoints_arg)
		return -EINVAL;

	const struct darling_sa_endpoints* ep = (const struct darling_sa_endpoints*) endpoints_arg;
	if (!ep->sae_dstaddr || ep->sae_dstaddrlen <= 0)
		return -EINVAL;

	if (ep->sae_srcaddr && ep->sae_srcaddrlen > 0)
	{
		long bind_ret = sys_bind(fd, ep->sae_srcaddr, ep->sae_srcaddrlen);
		if (bind_ret < 0 && bind_ret != -EINVAL)
			return bind_ret;
	}

	long ret = sys_connect_nocancel(fd, ep->sae_dstaddr, ep->sae_dstaddrlen);
	if (ret < 0 && ret != -EINPROGRESS)
		return ret;

	if (iov && iovcnt > 0)
	{
		// While the connection is still being established nothing can be
		// sent yet, and the number of bytes sent is reported as 0.
		long bytes = (ret == -EINPROGRESS) ? 0 : sys_writev(fd, iov, iovcnt);
		if (len)
		{
			*((__SIZE_TYPE__*) len) = (bytes > 0) ? bytes : 0;
		}
	}
	else if (len)
	{
		*((__SIZE_TYPE__*) len) = 0;
	}

	if (connid)
	{
		*((unsigned int*) connid) = 1;
	}

	return ret;
}

long sys_disconnectx(int fd, unsigned int associd, unsigned int connid)
{
	int type = 0, typelen = sizeof(type);
	long ret = sys_getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &typelen);
	if (ret < 0)
		return ret;

	// A stream connection is closed gracefully (FIN), as on macOS. Linux
	// aborts it with a reset when a TCP socket is connected to AF_UNSPEC.
	if (type == SOCK_STREAM)
	{
		ret = sys_shutdown(fd, SHUT_RDWR);
		return (ret < 0) ? ret : 0;
	}

	// Other sockets drop their peer address.
	struct sockaddr sa;
	__builtin_memset(&sa, 0, sizeof(sa));
	sa.sa_family = AF_UNSPEC;
	ret = sys_connect_nocancel(fd, &sa, sizeof(sa));
	return (ret < 0) ? ret : 0;
}
