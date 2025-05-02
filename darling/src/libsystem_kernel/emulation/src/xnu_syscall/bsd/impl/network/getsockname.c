#include <darling/emulation/xnu_syscall/bsd/impl/network/getsockname.h>

#include <sys/socket.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/duct.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>

extern __SIZE_TYPE__ strlen(const char* src);
extern char* strcpy(char* dest, const char* src);
extern char *strncpy(char *dest, const char *src, __SIZE_TYPE__ n);

long sys_getsockname(int fd, void* asa, int* socklen)
{
	int ret;
	struct sockaddr_fixup* fixed;

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_GETSOCKNAME,
			((long[6]) { fd, asa, socklen }));
#else
	ret = LINUX_SYSCALL(__NR_getsockname, fd, asa, socklen);
#endif

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else
	{
		fixed = (struct sockaddr_fixup*) asa;
		ret = *socklen = sockaddr_fixup_from_linux(fixed, asa, *socklen);
		if (ret > 0)
			ret = 0;
	}

	return ret;
}
