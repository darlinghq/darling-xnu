#include <darling/emulation/legacy_path/unistd/pread.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/bsdthread/cancelable.h>

long sys_pread(int fd, void* mem, int len, long long ofs)
{
	CANCELATION_POINT();
	return sys_pread_nocancel(fd, mem, len, ofs);
}

long sys_pread_nocancel(int fd, void* mem, int len, long long ofs)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_pread64, fd, mem, len, LL_ARG(ofs));
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}

