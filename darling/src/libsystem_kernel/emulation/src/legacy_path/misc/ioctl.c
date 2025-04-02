#include "ioctl.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>

long __real_ioctl(int fd, int req, void* arg)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_ioctl, fd, req, arg);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

long __real_ioctl_raw(int fd, int req, void* arg)
{
	return LINUX_SYSCALL(__NR_ioctl, fd, req, arg);
}
