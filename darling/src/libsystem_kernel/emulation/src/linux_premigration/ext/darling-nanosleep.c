/*
 * Copyright (C) 2026 Lubos Dolezel
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_register.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/linux_premigration/signal/sigexc.h>
#include <darling/emulation/linux_premigration/ext/darling-nanosleep.h>

extern uint64_t mach_absolute_time(void);

#define SLEEP_CLOCK_MONOTONIC 1
#define SLEEP_TIMER_ABSTIME 1

struct sleep_timespec
{
	long tv_sec;
	long tv_nsec;
};

// Darling's private entry point for nanosleep() and usleep(), which libc calls instead of waiting on a
// private semaphore that nothing signals (a darlingserver round trip per sleep). macOS has no such
// call, and __semwait_signal() with no semaphore still fails with EINVAL as it does there.
//
// cancelable: the sleep is a cancellation point and ends early (EINTR) when the thread is marked for
// cancellation. Returns 0 when the time has passed, EINTR only if the guest would see a signal: a signal handler ran in this thread, or the thread
// is marked for cancellation at a cancellation point. Linux also ends the sleep for signals that sigexc
// ignores or drops (and for darlingserver's own), which the wait must not report.
VISIBLE
int __darling_nanosleep(int cancelable, int64_t tv_sec, int32_t tv_nsec)
{
	struct sleep_timespec deadline;
	int clock = SLEEP_CLOCK_MONOTONIC;
	bool track_handlers = bsdthread_registered();
	long max_sec = (long)(~0UL >> 1);

	if (tv_sec < 0 || tv_nsec < 0 || tv_nsec >= 1000000000)
		return EINVAL;

	if (tv_sec == 0 && tv_nsec == 0)
		return 0;

	// an absolute deadline makes the retry after a signal that is not visible to the guest exact
	uint64_t now = mach_absolute_time();
	uint64_t ns = now % 1000000000ull + (uint64_t) tv_nsec;
	uint64_t sec = now / 1000000000ull + ns / 1000000000ull;

	if (tv_sec >= (int64_t) max_sec || sec > (uint64_t) max_sec - (uint64_t) tv_sec)
		deadline.tv_sec = max_sec;
	else
		deadline.tv_sec = (long) (sec + (uint64_t) tv_sec);
	deadline.tv_nsec = (long) (ns % 1000000000ull);

	for (;;)
	{
		unsigned long handlers = track_handlers ? sigexc_handlers_run() : 0;
		int ret;

		if (cancelable)
			ret = pthread_cancelable_syscall(clock, SLEEP_TIMER_ABSTIME, (long) &deadline, 0, 0, 0, __NR_clock_nanosleep);
		else
			ret = LINUX_SYSCALL(__NR_clock_nanosleep, clock, SLEEP_TIMER_ABSTIME, &deadline, NULL);

		if (ret == 0)
			return 0;
		if (ret != -LINUX_EINTR)
			return errno_linux_to_bsd(-ret);

		// without libpthread's TSD there is no way to tell, so report what Linux said
		if (!track_handlers || sigexc_handlers_run() != handlers)
			return EINTR;
		if (cancelable && pthread_testcancel_presyscall())
			return EINTR;
	}
}
