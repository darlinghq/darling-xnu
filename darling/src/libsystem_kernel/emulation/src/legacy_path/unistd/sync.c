#include "sync.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>

long sys_sync(void)
{
	int ret;

	ret = LINUX_SYSCALL0(__NR_sync);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

