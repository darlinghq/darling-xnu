#include <emulation/xnu_syscall/bsd/impl/unistd/chdir.h>

#include <emulation/darling/server/vchroot_userspace.h>
#include <emulation/darling/tsd/perthread_wd.h>
#include <emulation/linux_api/linux_syscall.h>

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
