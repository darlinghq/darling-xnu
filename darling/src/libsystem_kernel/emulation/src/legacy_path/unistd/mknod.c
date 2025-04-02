#include "mknod.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>
#include <mach/lkm.h>
#include "../vchroot_expand.h"
#include "../bsdthread/per_thread_wd.h"

long sys_mknod(const char* path, int mode, int dev)
{
	int ret;
	struct vchroot_expand_args vc;

	vc.flags = 0;
	vc.dfd = get_perthread_wd();
	
	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	#if defined(__NR_mknod)
		ret = LINUX_SYSCALL(__NR_mknod, vc.path, mode, dev);
	#else
		ret = LINUX_SYSCALL(__NR_mknodat, LINUX_AT_FDCWD, vc.path, mode, dev);
	#endif
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
