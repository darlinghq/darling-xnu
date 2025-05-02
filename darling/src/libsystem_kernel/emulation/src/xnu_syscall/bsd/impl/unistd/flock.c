#include <darling/emulation/xnu_syscall/bsd/impl/unistd/flock.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_flock(int fd, int op)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_flock, fd, op);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
