#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchmod.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_fchmod(int fd, int mode)
{
	int ret;

	ret = LINUX_SYSCALL2(__NR_fchmod, fd, mode);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
