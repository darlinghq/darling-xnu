#include <emulation/xnu_syscall/bsd/impl/unistd/write.h>

#include <emulation/linux_api/linux_syscall.h>
#include <emulation/xnu_syscall/shared/bsdthread/cancelable.h>

long sys_write(int fd, const void* mem, int len)
{
	CANCELATION_POINT();
	return sys_write_nocancel(fd, mem, len);
}

long sys_write_nocancel(int fd, const void* mem, int len)
{
	int ret;

	ret = LINUX_SYSCALL3(__NR_write, fd, mem, len);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}
