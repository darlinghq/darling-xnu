#include <mach/mach_time.h>
#include <mach/kern_return.h>
#include <errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/ext/sys/linux_time.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/gettimeofday.h>

#ifndef NSEC_PER_SEC
#	define NSEC_PER_SEC 1000000000ull
#endif

kern_return_t		mach_timebase_info_trap_impl(
						mach_timebase_info_t	info)
{
	info->numer = 1;
	info->denom = 1;
	return KERN_SUCCESS;
}

kern_return_t		mach_wait_until_impl(
						uint64_t		deadline)
{
	struct timespec ts = { deadline / NSEC_PER_SEC, deadline % NSEC_PER_SEC };
	
do_sleep:
	if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, 0) == -1)
	{
		if (errno == EINTR)
			goto do_sleep;
		return KERN_FAILURE;
	}
	
	return KERN_SUCCESS;
}
