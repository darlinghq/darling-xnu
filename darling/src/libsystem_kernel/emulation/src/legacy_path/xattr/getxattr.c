#include <darling/emulation/legacy_path/xattr/getxattr.h>

#include <sys/errno.h>

extern char* strcpy(char* dst, const char* src);

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/vchroot_expand.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

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
