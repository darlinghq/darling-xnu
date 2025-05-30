#include <darling/emulation/linux_premigration/ext/sys/epoll.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern long cerror(int __err);

VISIBLE
int epoll_create (int __size)
{
	#if defined(__NR_epoll_create)
		int rv;

		rv = LINUX_SYSCALL(__NR_epoll_create, __size);
		if (rv < 0)
		{
			cerror(errno_linux_to_bsd(-rv));
			return -1;
		}

		return rv;
	#else
		// The size argument in __NR_epoll_create is ignored
		return epoll_create1(0);
	#endif
}
