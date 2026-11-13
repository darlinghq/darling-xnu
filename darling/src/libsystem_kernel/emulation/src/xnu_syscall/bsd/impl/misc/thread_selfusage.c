// Copyright (C) 2026 Lubos Dolezel
#include <darling/emulation/xnu_syscall/bsd/impl/misc/thread_selfusage.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define LINUX_CLOCK_THREAD_CPUTIME_ID 3

// The CPU time the calling thread has consumed, in mach absolute time units.
// Those are nanoseconds here (mach_timebase_info reports 1/1). The call cannot
// fail; if the clock is unavailable the usage reads as zero. The result is 64
// bits wide, so it is returned as long long: 32-bit callers get it in %edx:%eax,
// as with lseek.
VISIBLE // made visible for xtrace
long long sys_thread_selfusage(void)
{
	struct { long tv_sec; long tv_nsec; } ts;

	if (LINUX_SYSCALL(__NR_clock_gettime, LINUX_CLOCK_THREAD_CPUTIME_ID, &ts) < 0)
		return 0;

	return (long long) ts.tv_sec * 1000000000LL + ts.tv_nsec;
}
