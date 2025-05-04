#include <darling/emulation/linux_premigration/ext/sys/epoll.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern long cerror(int __err);

VISIBLE
int epoll_create1 (int __flags)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_epoll_create1, __flags);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}
