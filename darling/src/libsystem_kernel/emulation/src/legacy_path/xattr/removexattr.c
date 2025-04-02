#include "removexattr.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>

#define XATTR_NOFOLLOW	1

long sys_removexattr(const char* path, const char* name, int options)
{
	int ret;

	if (options & XATTR_NOFOLLOW)
		ret = LINUX_SYSCALL(__NR_lremovexattr, path, name);
	else
		ret = LINUX_SYSCALL(__NR_removexattr, path, name);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}
