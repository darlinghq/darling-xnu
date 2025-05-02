#include <darling/emulation/xnu_syscall/bsd/impl/unistd/pwrite.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

long sys_pwrite(int fd, const void* mem, int len, long long ofs)
{
	CANCELATION_POINT();
	return sys_pwrite_nocancel(fd, mem, len, ofs);
}

long sys_pwrite_nocancel(int fd, const void* mem, int len, long long ofs)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_pwrite64, fd, mem, len, LL_ARG(ofs));
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}
