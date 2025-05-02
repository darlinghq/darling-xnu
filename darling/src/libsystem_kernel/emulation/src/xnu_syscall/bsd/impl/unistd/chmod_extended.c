#include <darling/emulation/xnu_syscall/bsd/impl/unistd/chmod_extended.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>

long sys_chmod_extended(const char* path, int uid, int gid, int mode, void* xsec)
{
	int ret;

	struct vchroot_expand_args vc;
	vc.flags = VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();

	strcpy(vc.path, path);
	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	#if defined(__NR_chmod)
		ret = LINUX_SYSCALL(__NR_chmod, vc.path, mode);
	#else
		ret = LINUX_SYSCALL(__NR_fchmodat, LINUX_AT_FDCWD, vc.path, mode, 0);
	#endif
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	#if defined(__NR_chown)
		ret = LINUX_SYSCALL(__NR_chown, vc.path, uid, gid);
	#else
		ret = LINUX_SYSCALL(__NR_fchownat, LINUX_AT_FDCWD, vc.path, uid, gid, 0);
	#endif
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}
