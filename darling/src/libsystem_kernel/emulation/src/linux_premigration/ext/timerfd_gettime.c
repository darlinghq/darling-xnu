#include <darling/emulation/linux_premigration/ext/sys/timerfd.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern long cerror(int __err);

VISIBLE
int timerfd_gettime (int __fd, struct itimerspec* __otmr)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_timerfd_gettime, __fd, __otmr);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}
