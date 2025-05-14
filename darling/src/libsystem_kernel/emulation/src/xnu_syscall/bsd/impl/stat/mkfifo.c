#include <darling/emulation/xnu_syscall/bsd/impl/stat/mkfifo.h>

#include <sys/errno.h>

#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/other/mach/lkm.h>

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
