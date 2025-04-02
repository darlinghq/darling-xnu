#include "mkfifo.h"
#include "common.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>
#include "../bsdthread/per_thread_wd.h"
#include "../vchroot_expand.h"
#include <mach/lkm.h>
#include <sys/errno.h>

#define LINUX_S_IFIFO 0010000

long sys_mkfifo(const char* path, unsigned int mode)
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

	#if defined(__NR_mknod)
		ret = LINUX_SYSCALL(__NR_mknod, vc.path, mode | LINUX_S_IFIFO, 0);
	#else
		ret = LINUX_SYSCALL(__NR_mknodat, LINUX_AT_FDCWD, vc.path, mode | LINUX_S_IFIFO, 0);
	#endif

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return 0;
}
