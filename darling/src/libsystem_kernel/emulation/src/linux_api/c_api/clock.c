#include <emulation/linux_api/c_api/clock.h>

#include <emulation/linux_api/linux_syscall.h>

extern long cerror(int err);

VISIBLE
int linux_clock_nanosleep(int clockid, int flags,
		const struct timespec* req, struct timespec* rem)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_clock_nanosleep, clockid, flags, req, rem);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}

VISIBLE
int linux_clock_gettime(int clockid, struct timespec* res)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_clock_gettime, clockid, res);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}
