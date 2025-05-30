#include <darling/emulation/xnu_syscall/bsd/impl/network/accept.h>

#include <sys/fcntl.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/conversion/network/socket.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/network/socket.h>
#include <darling/emulation/xnu_syscall/bsd/helper/network/duct.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>

long sys_accept(int fd, void* from, int* socklen)
{
	CANCELATION_POINT();
	return sys_accept_nocancel(fd, from, socklen);
}

long sys_accept_nocancel(int fd, void* from, int* socklen)
{
	int ret;
	struct sockaddr_fixup* fixed;

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_ACCEPT, ((long[6]) { fd, from, socklen }));
#else
	ret = LINUX_SYSCALL(__NR_accept, fd, from, socklen);
#endif

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else
	{
		fixed = (struct sockaddr_fixup*) from;
		fixed->bsd_family = sfamily_linux_to_bsd(fixed->linux_family);
		fixed->bsd_length = *socklen;

		// on macOS (and BSD), accept-ed sockets inherit O_NONBLOCK
		int parent_flags = sys_fcntl_nocancel(fd, F_GETFL, 0);
		int child_flags = sys_fcntl_nocancel(ret, F_GETFL, 0);
		// we silently ignore errors we get from fcntl (it's probably fine; fnctl shouldn't fail here)
		if (parent_flags >= 0 && child_flags >= 0 && (parent_flags & BSD_O_NONBLOCK) != 0) {
			sys_fcntl_nocancel(ret, F_SETFL, child_flags | BSD_O_NONBLOCK);
		}
	}

	return ret;
}
