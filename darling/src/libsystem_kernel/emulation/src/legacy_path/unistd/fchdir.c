#include <darling/emulation/legacy_path/unistd/fchdir.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_fchdir(int fd)
{
	int ret;

	ret = LINUX_SYSCALL1(__NR_fchdir, fd);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
