#include <darling/emulation/xnu_syscall/bsd/impl/xattr/fgetxattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/xattr_utils.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_fgetxattr(int fd, const char* name, char* value,
		unsigned long size, unsigned int pos, int options)
{
	int ret;
	char linux_name[XATTR_LINUX_NAME_BUFSIZE];
	
	if (pos != 0)
		return -ERANGE;

	ret = xattr_name_to_linux(name, linux_name);
	if (ret < 0)
		return ret;

	// a NULL value asks for the size of the attribute, whatever size says
	if (value == NULL)
		size = 0;

	ret = LINUX_SYSCALL(__NR_fgetxattr, fd, linux_name, value, size);

	if (ret < 0)
		return errno_linux_xattr_to_bsd(ret);

	return ret;
}
