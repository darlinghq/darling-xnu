#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getxattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/xattr_utils.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_getxattr(const char* path, const char* name, char* value,
		unsigned long size, unsigned int pos, int options)
{
	int ret;
	char l_name[XATTR_NAME_MAX_LEN];

	if (pos != 0)
		return -ERANGE;

	struct vchroot_expand_args vc;
	vc.flags = (options & DARWIN_XATTR_NOFOLLOW) ? 0 : VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();

	__simple_snprintf(vc.path, sizeof(vc.path), "%s", path);
	ret = vchroot_expand(&vc);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	const char* linux_name = xattr_name_to_linux(name, l_name, sizeof(l_name));

	if (options & DARWIN_XATTR_NOFOLLOW)
		ret = LINUX_SYSCALL(__NR_lgetxattr, vc.path, linux_name, value, size);
	else
		ret = LINUX_SYSCALL(__NR_getxattr, vc.path, linux_name, value, size);

	if (ret < 0)
		return errno_linux_xattr_to_bsd(ret);

	return ret;
}
