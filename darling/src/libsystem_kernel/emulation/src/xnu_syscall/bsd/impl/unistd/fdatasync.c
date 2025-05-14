#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fdatasync.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_fdatasync(int fd)
{
	int ret;

	ret = LINUX_SYSCALL1(__NR_fdatasync, fd);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
