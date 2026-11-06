#include <darling/emulation/xnu_syscall/bsd/impl/time/gettimeofday.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/ext/sys/linux_time.h>

long sys_gettimeofday(struct bsd_timeval* tv, struct timezone* tz)
{
	int ret;
	struct linux_timeval ltv;

	// Linux computes gettimeofday() from CLOCK_REALTIME, truncated to microseconds;
	// the vDSO gives the same value without a system call. The time zone needs the system call.
	if (tz == NULL && tv != NULL)
	{
		struct timespec ts;

		if (__darling_vdso_clock_gettime(CLOCK_REALTIME, &ts) == 0)
		{
			tv->tv_sec = ts.tv_sec;
			tv->tv_usec = ts.tv_nsec / 1000;
			return 0;
		}
	}

	ret = LINUX_SYSCALL(__NR_gettimeofday, &ltv, tz);
	if (ret < 0)
	{
		ret = errno_linux_to_bsd(ret);
	}
	else
	{
		tv->tv_sec = ltv.tv_sec;
		tv->tv_usec = ltv.tv_usec;
	}

	return ret;
}
