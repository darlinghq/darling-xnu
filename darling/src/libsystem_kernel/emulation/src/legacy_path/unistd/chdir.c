#include <darling/emulation/legacy_path/unistd/chdir.h>

#include <darling/emulation/legacy_path/mach/lkm.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/vchroot_expand.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

long sys_chdir(const char* path)
{
	int ret;
	struct vchroot_expand_args vc;

	vc.flags = VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();
	
	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	ret = LINUX_SYSCALL(__NR_chdir, vc.path);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
