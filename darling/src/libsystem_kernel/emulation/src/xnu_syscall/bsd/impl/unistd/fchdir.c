#include <emulation/xnu_syscall/bsd/impl/unistd/fchdir.h>

#include <emulation/linux_api/linux_syscall.h>

long sys_fchdir(int fd)
{
	int ret;

	ret = LINUX_SYSCALL1(__NR_fchdir, fd);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
