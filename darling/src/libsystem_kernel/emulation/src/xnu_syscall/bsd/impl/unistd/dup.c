#include <emulation/xnu_syscall/bsd/impl/unistd/dup.h>

#include <emulation/linux_api/linux_syscall.h>

extern void kqueue_dup(int oldfd, int newfd);

long sys_dup(int fd)
{
	int ret;

	ret = LINUX_SYSCALL1(__NR_dup, fd);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else
		kqueue_dup(fd, ret);

	return ret;
}

