#include <darling/emulation/legacy_path/unistd/fchmod.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_fchmod(int fd, int mode)
{
	int ret;

	ret = LINUX_SYSCALL2(__NR_fchmod, fd, mode);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

