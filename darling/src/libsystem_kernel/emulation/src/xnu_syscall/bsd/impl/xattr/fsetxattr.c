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
	char l_name[XATTR_NAME_MAX_LEN];
	int l_flags;

	if (pos != 0)
		return -ERANGE;

	const char* linux_name = xattr_name_to_linux(name, l_name, sizeof(l_name));
	l_flags = xattr_options_to_linux_flags(options);

	ret = LINUX_SYSCALL(__NR_fsetxattr, fd, linux_name, value, size, l_flags);

	if (ret < 0)
		return errno_linux_xattr_to_bsd(ret);

	return ret;
}
