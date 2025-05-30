#include <darling/emulation/xnu_syscall/bsd/impl/network/sendto.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/network/duct.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/duct.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

extern void *memcpy(void *dest, const void *src, __SIZE_TYPE__ n);

long sys_sendto(int fd, const void* buffer, unsigned long length,
		int flags, const void* sockaddr, int socklen)
{
	CANCELATION_POINT();
	return sys_sendto_nocancel(fd, buffer, length, flags, sockaddr, socklen);
}

long sys_sendto_nocancel(int fd, const void* buffer, unsigned long length,
		int flags, const void* sockaddr, int socklen)
{
	int ret, linux_flags;

	struct sockaddr_fixup* fixed = NULL;

	if (socklen > 512)
		return -EINVAL;

	if (sockaddr != NULL) {
		fixed = __builtin_alloca(sockaddr_fixup_size_from_bsd(sockaddr, socklen));
		ret = socklen = sockaddr_fixup_from_bsd(fixed, sockaddr, socklen);
		if (ret < 0)
			return ret;
	}

	linux_flags = msgflags_bsd_to_linux(flags);

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_SENDTO,
			((long[6]) { fd, buffer, length, linux_flags, fixed, socklen }));
#else
	ret = LINUX_SYSCALL(__NR_sendto, fd, buffer, length,
			linux_flags, fixed, socklen);
#endif

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
