#include <mach/mach_time.h>
#include <mach/kern_return.h>
#include <errno.h>

#include <emulation/common/base.h>
#include <emulation/linux_api/c_api/clock.h>

#ifndef NSEC_PER_SEC
#	define NSEC_PER_SEC 1000000000ull
#endif

kern_return_t mach_timebase_info_trap_impl(mach_timebase_info_t info)
{
	info->numer = 1;
	info->denom = 1;
	return KERN_SUCCESS;
}

kern_return_t mach_wait_until_impl(uint64_t deadline)
{
	struct timespec ts = { deadline / NSEC_PER_SEC, deadline % NSEC_PER_SEC };
	
do_sleep:
	if (linux_clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, 0) == -1)
	{
		if (errno == EINTR)
			goto do_sleep;
		return KERN_FAILURE;
	}
	
	return KERN_SUCCESS;
}

// TODO: Find a better location to keep this in

// Instead of using Apple's offical implementation of mach_absolute_time (in
// `mach_absolute_time.S`), we will use our own custom implementation.

VISIBLE
uint64_t mach_absolute_time(void)
{
	struct timespec ts;
	uint64_t out;
	
	linux_clock_gettime(CLOCK_MONOTONIC, &ts);
	
	out = ts.tv_nsec;
	out += ts.tv_sec * NSEC_PER_SEC;
	
	return out;
}
