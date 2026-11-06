#include <darling/emulation/xnu_syscall/bsd/impl/xattr/listxattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/xattr_utils.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

long sys_listxattr(const char* path, char* namebuf, unsigned long size, int options)
{
	int ret;
	struct vchroot_expand_args vc;

	vc.flags = (options & DARWIN_XATTR_NOFOLLOW) ? 0 : VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();
	
	__simple_snprintf(vc.path, sizeof(vc.path), "%s", path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	if (options & DARWIN_XATTR_NOFOLLOW)
		return xattr_list_to_darwin(__NR_llistxattr, (long) vc.path, namebuf, size);
	else
		return xattr_list_to_darwin(__NR_listxattr, (long) vc.path, namebuf, size);
}
