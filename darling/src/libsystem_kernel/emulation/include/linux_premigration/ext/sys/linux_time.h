#ifndef EXT_TIME_H
#define EXT_TIME_H

#include "timerfd.h"

#ifndef CLOCK_REALTIME
#	define CLOCK_REALTIME	0
#	define CLOCK_MONOTONIC	1
/* TODO: Other clocks */

#endif
#define TIMER_ABSTIME	1

#define clock_gettime		__linux_clock_gettime
#define clock_nanosleep		__linux_clock_nanosleep

extern int clock_gettime(int __clockid, struct timespec* __res);

extern int clock_nanosleep(int __clockid, int __flags,
		const struct timespec* __req, struct timespec* __rem);

// Reads a clock through the host kernel's vDSO (x86_64 only).
// Returns 0 on success, -1 if the caller has to make the system call instead.
int __darling_vdso_clock_gettime(int __clockid, struct timespec* __res);

// Looks up the vDSO entry point; called from mach_driver_init()
void __darling_vdso_clock_init(void);

#endif // EXT_TIME_H
