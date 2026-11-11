#include <darling/emulation/xnu_syscall/bsd/impl/time/futimes.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/time/utimes.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>


long sys_futimes(int fd, struct bsd_timeval* tv)
{
	int ret;
	struct linux_utimensat_timespec lts[2];

	if (tv)
		utimes_timeval_to_timespec(tv, lts);

	ret = LINUX_SYSCALL(__NR_utimensat, fd, NULL, (tv ? lts : NULL), 0);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
