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

	int call_nr = (options & DARWIN_XATTR_NOFOLLOW) ? __NR_llistxattr : __NR_listxattr;

	char stack_buf[1024];
	char* l_buf = NULL;
	long l_size = fetch_linux_xattr_list(call_nr, vc.path, -1, &l_buf, stack_buf, sizeof(stack_buf));
	if (l_size <= 0)
		return l_size;

	long result = translate_linux_xattr_list(l_buf, l_size, namebuf, size);
	if (l_buf && l_buf != stack_buf)
		free(l_buf);

	return result;
}
