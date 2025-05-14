#include <darling/emulation/linux_premigration/misc/ioctl.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long __real_ioctl(int fd, int req, void* arg)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_ioctl, fd, req, arg);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
