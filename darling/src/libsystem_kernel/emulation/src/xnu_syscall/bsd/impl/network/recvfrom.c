#include <darling/emulation/xnu_syscall/bsd/impl/network/recvfrom.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/network/duct.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/socket.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/duct.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

extern void *memcpy(void *dest, const void *src, __SIZE_TYPE__ n);

long sys_recvfrom(int fd, void* buf, unsigned long len,
		int flags, void* from, int* socklen)
{
	CANCELATION_POINT();
	return sys_recvfrom_nocancel(fd, buf, len, flags, from, socklen);
}

long sys_recvfrom_nocancel(int fd, void* buf, unsigned long len,
		int flags, void* from, int* socklen)
{
	int ret, linux_flags;
	struct sockaddr_fixup* fixed;

	linux_flags = msgflags_bsd_to_linux(flags);

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_RECVFROM, ((long[6]) { fd, buf, len,
			flags, from, socklen }));
#else
	ret = LINUX_SYSCALL(__NR_recvfrom, fd, buf, len, flags, from, socklen);
#endif

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else if (from != NULL)
	{
		fixed = (struct sockaddr_fixup*) from;
		if ((*socklen = sockaddr_fixup_from_linux(fixed, from, *socklen)) < 0)
			ret = *socklen;
	}

	return ret;
}
