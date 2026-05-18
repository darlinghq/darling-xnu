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
