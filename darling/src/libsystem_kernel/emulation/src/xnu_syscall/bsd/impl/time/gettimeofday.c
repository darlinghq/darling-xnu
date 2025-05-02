#include <darling/emulation/xnu_syscall/bsd/impl/time/gettimeofday.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_gettimeofday(struct bsd_timeval* tv, struct timezone* tz)
{
	int ret;
	struct linux_timeval ltv;

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
