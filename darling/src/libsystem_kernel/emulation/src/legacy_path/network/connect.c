#include <darling/emulation/legacy_path/network/connect.h>

#include <sys/socket.h>
#include <sys/errno.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/network/duct.h>
#include <darling/emulation/legacy_path/bsdthread/cancelable.h>

extern void *memcpy(void *dest, const void *src, __SIZE_TYPE__ n);
extern __SIZE_TYPE__ strlen(const char* src);
extern char* strcpy(char* dest, const char* src);
extern char *strncpy(char *dest, const char *src, __SIZE_TYPE__ n);

// Must be included after strncpy
#include <darling/emulation/legacy_path/vchroot_expand.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

long sys_connect(int fd, const void* name, int socklen)
{
	CANCELATION_POINT();
	return sys_connect_nocancel(fd, name, socklen);
}

long sys_connect_nocancel(int fd, const void* name, int socklen)
{
	int ret;
	int linux_domain;
	struct sockaddr_fixup* fixed;

	if (socklen > 512)
		return -EINVAL;

	fixed = __builtin_alloca(sockaddr_fixup_size_from_bsd(name, socklen));
	ret = socklen = sockaddr_fixup_from_bsd(fixed, name, socklen);
	if (ret < 0)
		return ret;

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_CONNECT, ((long[6]) { fd, fixed, socklen }));
#else
	ret = LINUX_SYSCALL(__NR_connect, fd, fixed, socklen);
#endif

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

