#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fsync.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_fsync(int fd)
{
	return sys_fsync_nocancel(fd);
}

long sys_fsync_nocancel(int fd)
{
	int ret;

	ret = LINUX_SYSCALL1(__NR_fsync, fd);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
