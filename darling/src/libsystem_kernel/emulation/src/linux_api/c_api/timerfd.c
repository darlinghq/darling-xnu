#include <emulation/linux_api/c_api/sys/timerfd.h>

#include <emulation/linux_api/linux_syscall.h>

extern long cerror(int err);

VISIBLE
int linux_timerfd_create(int clock_id, int flags)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_timerfd_create, clock_id, flags);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}

VISIBLE
int linux_timerfd_gettime(int fd, struct itimerspec* otmr)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_timerfd_gettime, fd, otmr);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}

VISIBLE
int linux_timerfd_settime(int fd, int flags, const struct itimerspec* utmr, struct itimerspec* otmr)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_timerfd_settime, fd, flags, utmr, otmr);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}