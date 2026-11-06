#include <darling/emulation/xnu_syscall/bsd/impl/time/futimes.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>


long sys_futimes(int fd, struct bsd_timeval* tv)
{
	int ret;
	struct {
		long tv_sec;
		long tv_nsec;
	} lts[2];

	if (tv) {
		for (int i = 0; i < 2; i++) {
			lts[i].tv_sec = tv[i].tv_sec;
			lts[i].tv_nsec = tv[i].tv_usec >= 0 && tv[i].tv_usec < 1000000
				? (long)tv[i].tv_usec * 1000 : -1;
		}
	}

	ret = LINUX_SYSCALL(__NR_utimensat, fd, NULL, (tv ? lts : NULL), 0);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
