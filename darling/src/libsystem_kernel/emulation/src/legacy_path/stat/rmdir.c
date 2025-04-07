#include <darling/emulation/legacy_path/stat/rmdir.h>

#include <sys/errno.h>

#include <darling/emulation/legacy_path/stat/common.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>
#include <darling/emulation/legacy_path/vchroot_expand.h>
#include <darling/emulation/legacy_path/mach/lkm.h>

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
