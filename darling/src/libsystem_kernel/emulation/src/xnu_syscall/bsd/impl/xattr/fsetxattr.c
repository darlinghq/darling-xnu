#include <darling/emulation/xnu_syscall/bsd/impl/xattr/fsetxattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/xattr_utils.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_fsetxattr(int fd, const char* name, const void* value,
		unsigned long size, unsigned int pos, int options)
{
	int ret;
	char linux_name[XATTR_LINUX_NAME_BUFSIZE];
	int l_flags;

	if (pos != 0)
		return -ERANGE;

	ret = xattr_name_to_linux(name, linux_name);
	if (ret < 0)
		return ret;

	l_flags = xattr_options_to_linux_flags(options);

	ret = LINUX_SYSCALL(__NR_fsetxattr, fd, linux_name, value, size, l_flags);

	if (ret < 0)
		return errno_linux_xattr_to_bsd(ret);

	return ret;
}
