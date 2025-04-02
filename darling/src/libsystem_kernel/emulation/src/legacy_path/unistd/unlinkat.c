#include "unlinkat.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>
#include "../common_at.h"
#include "../vchroot_expand.h"
#include <mach/lkm.h>
#include <sys/errno.h>

extern char* strcpy(char* dst, const char* src);

long sys_unlinkat(int fd, const char* path, int flag)
{
	int ret;
	struct vchroot_expand_args vc;
	int linux_flags;

	vc.flags = 0;
	vc.dfd = atfd(fd);
	
	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	linux_flags = atflags_bsd_to_linux(flag);
	if (linux_flags == LINUX_AT_INVALID)
		return -EINVAL;

	ret = LINUX_SYSCALL(__NR_unlinkat, vc.dfd, vc.path, linux_flags);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
