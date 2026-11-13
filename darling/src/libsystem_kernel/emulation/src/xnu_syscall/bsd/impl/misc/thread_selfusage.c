// Copyright (C) 2026 Lubos Dolezel
#include <darling/emulation/xnu_syscall/bsd/impl/misc/thread_selfusage.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define LINUX_CLOCK_THREAD_CPUTIME_ID 3

// The CPU time the calling thread has consumed, in mach absolute time units.
// Those are nanoseconds here (mach_timebase_info reports 1/1). The call cannot
// fail; if the clock is unavailable the usage reads as zero.
VISIBLE // made visible for xtrace
long sys_thread_selfusage(void)
{
	struct { long tv_sec; long tv_nsec; } ts;

	if (LINUX_SYSCALL(__NR_clock_gettime, LINUX_CLOCK_THREAD_CPUTIME_ID, &ts) < 0)
		return 0;

	return ts.tv_sec * 1000000000L + ts.tv_nsec;
}
