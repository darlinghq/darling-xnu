#ifndef DARLING_EMULATION_LINUX_API_CLOCK_H
#define DARLING_EMULATION_LINUX_API_CLOCK_H

#include <emulation/linux_api/c_api/sys/timerfd.h>

#ifndef CLOCK_REALTIME
#	define CLOCK_REALTIME	0
#	define CLOCK_MONOTONIC	1
/* TODO: Other clocks */
#endif

#define TIMER_ABSTIME	1

extern int linux_clock_gettime(int __clockid, struct timespec* __res);

extern int linux_clock_nanosleep(int __clockid, int __flags,
		const struct timespec* __req, struct timespec* __rem);

#endif // DARLING_EMULATION_LINUX_API_CLOCK_H
