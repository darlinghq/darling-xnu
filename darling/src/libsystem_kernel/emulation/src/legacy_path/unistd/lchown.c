#include <darling/emulation/legacy_path/unistd/lchown.h>

#include <darling/emulation/legacy_path/mach/lkm.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/vchroot_expand.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

long sys_lchown(const char* path, int uid, int gid)
{
#if 0
	int ret;
	struct vchroot_expand_args vc;

	vc.flags = 0;
	vc.dfd = get_perthread_wd();
	
	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

#ifdef __NR_chown32
	ret = LINUX_SYSCALL(__NR_lchown32, vc.path, uid, gid);
#else
	ret = LINUX_SYSCALL(__NR_lchown, vc.path, uid, gid);
#endif
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
#else
	return 0;
#endif
}
