#include <emulation/linux_api/c_api/ioctl.h>

#include <emulation/linux_api/linux_syscall.h>

long linux_ioctl(int fd, int req, void* arg)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_ioctl, fd, req, arg);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

long linux_ioctl_raw(int fd, int req, void* arg)
{
	return LINUX_SYSCALL(__NR_ioctl, fd, req, arg);
}
