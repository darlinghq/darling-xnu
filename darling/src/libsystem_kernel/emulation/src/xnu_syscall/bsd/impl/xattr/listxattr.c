#include <darling/emulation/xnu_syscall/bsd/impl/xattr/listxattr.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

#define XATTR_NOFOLLOW	1

long sys_listxattr(const char* path, char* namebuf, unsigned long size, int options)
{
	int ret;
	struct vchroot_expand_args vc;

	vc.flags = (options & XATTR_NOFOLLOW) ? 0 : VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();
	
	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	if (options & XATTR_NOFOLLOW)
		ret = LINUX_SYSCALL(__NR_llistxattr, path, namebuf, size);
	else
		ret = LINUX_SYSCALL(__NR_listxattr, path, namebuf, size);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}
