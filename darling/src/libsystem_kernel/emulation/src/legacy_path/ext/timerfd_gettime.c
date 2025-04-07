#include <darling/emulation/legacy_path/ext/sys/timerfd.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

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

