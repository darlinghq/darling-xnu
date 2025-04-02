#include "rmdir.h"
#include "common.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>
#include "../bsdthread/per_thread_wd.h"
#include "../vchroot_expand.h"
#include <mach/lkm.h>
#include <sys/errno.h>

#define LINUX_AT_REMOVEDIR	0x200

long sys_rmdir(const char* path)
{
	int ret;
	struct vchroot_expand_args vc;

	if (!path)
		return -EFAULT;

	vc.flags = 0;
	vc.dfd = get_perthread_wd();
	
	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	ret = LINUX_SYSCALL(__NR_unlinkat, vc.dfd, vc.path, LINUX_AT_REMOVEDIR);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return 0;
}
