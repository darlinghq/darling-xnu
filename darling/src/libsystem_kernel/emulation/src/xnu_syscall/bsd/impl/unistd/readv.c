#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readv.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

long sys_readv(int fd, struct iovec* iovp, unsigned int len)
{
	CANCELATION_POINT();
	return sys_readv_nocancel(fd, iovp, len);
}

long sys_readv_nocancel(int fd, struct iovec* iovp, unsigned int len)
{
	int ret;

	ret = LINUX_SYSCALL3(__NR_readv, fd, iovp, len);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}
