#include "getpid.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>

long sys_getpid(void)
{
	int ret;

	ret = LINUX_SYSCALL0(__NR_getpid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

