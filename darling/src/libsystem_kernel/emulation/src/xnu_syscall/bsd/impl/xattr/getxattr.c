#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getxattr.h>

#include <sys/errno.h>

extern char* strcpy(char* dst, const char* src);

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/xattr/getxattr.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_getxattr(const char* path, const char* name, char* value,
		unsigned long size, unsigned int pos, int options)
{
	int ret;

	if (pos != 0)
		return -ERANGE;

	struct vchroot_expand_args vc;
	vc.flags = (options & XATTR_NOFOLLOW) ? 0 : VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();

	strcpy(vc.path, path);
	ret = vchroot_expand(&vc);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	ret = LINUX_SYSCALL(__NR_getxattr, vc.path, name, value, size);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return 0;
}
