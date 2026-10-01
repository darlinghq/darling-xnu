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

long sys_connect(int fd, const void* name, int socklen)
{
	CANCELATION_POINT();
	return sys_connect_nocancel(fd, name, socklen);
}

long sys_connect_nocancel(int fd, const void* name, int socklen)
{
	int ret;
	struct sockaddr_fixup* fixed;

	if (socklen > 512)
		return -EINVAL;

	fixed = __builtin_alloca(sockaddr_fixup_size_from_bsd(name, socklen));
	ret = socklen = sockaddr_fixup_from_bsd(fixed, name, socklen);
	if (ret < 0)
		return ret;

	// DARLING workaround: Darwin clients on Darling-arm64 often set
	// O_NONBLOCK on the socket and then wait for connect completion via
	// Mach kqueue paths that Darling's kqueue plumbing doesn't yet wire
	// through to Linux socket-fd readiness events. Temporarily clear
	// O_NONBLOCK so the underlying Linux connect() blocks until the TCP
	// handshake completes, then restore the original flags. The caller
	// sees ret=0 instead of EINPROGRESS — close enough for the typical
	// "connect then send" sequence to work without polling.
	long flags = LINUX_SYSCALL(__NR_fcntl, fd, 3 /*F_GETFL*/, 0);
	int was_nonblock = (flags >= 0) && (flags & 0x800 /*O_NONBLOCK*/);
	if (was_nonblock) {
		LINUX_SYSCALL(__NR_fcntl, fd, 4 /*F_SETFL*/, flags & ~0x800L);
	}

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_CONNECT, ((long[6]) { fd, fixed, socklen }));
#else
	ret = LINUX_SYSCALL(__NR_connect, fd, fixed, socklen);
#endif

	if (was_nonblock) {
		LINUX_SYSCALL(__NR_fcntl, fd, 4 /*F_SETFL*/, flags);  // restore O_NONBLOCK
	}

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

#include <darling/emulation/xnu_syscall/bsd/impl/network/bind.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/writev.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/shutdown.h>

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
		long bytes = sys_writev(fd, iov, iovcnt);
		if (bytes > 0 && len)
		{
			*((__SIZE_TYPE__*) len) = bytes;
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
	struct sockaddr sa;
	__builtin_memset(&sa, 0, sizeof(sa));
	sa.sa_family = AF_UNSPEC;
	long ret = sys_connect_nocancel(fd, &sa, sizeof(sa));
	if (ret < 0)
	{
		ret = sys_shutdown(fd, 2 /* SHUT_RDWR */);
	}
	return (ret < 0) ? ret : 0;
}
