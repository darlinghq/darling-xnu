#include <darling/emulation/legacy_path/unistd/pwrite.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/bsdthread/cancelable.h>

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

