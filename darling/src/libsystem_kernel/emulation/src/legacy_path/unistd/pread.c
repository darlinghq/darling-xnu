#include "pread.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>
#include "../bsdthread/cancelable.h"

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

