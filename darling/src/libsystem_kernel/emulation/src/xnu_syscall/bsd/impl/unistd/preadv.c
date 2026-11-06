#include <darling/emulation/xnu_syscall/bsd/impl/unistd/preadv.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

long sys_preadv(int fd, struct iovec* iovp, unsigned int len, long long ofs)
{
	CANCELATION_POINT();
	return sys_preadv_nocancel(fd, iovp, len, ofs);
}

long sys_preadv_nocancel(int fd, struct iovec* iovp, unsigned int len, long long ofs)
{
	long ret;

	ret = LINUX_SYSCALL(__NR_preadv, fd, iovp, len, LL_ARG(ofs));
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}
