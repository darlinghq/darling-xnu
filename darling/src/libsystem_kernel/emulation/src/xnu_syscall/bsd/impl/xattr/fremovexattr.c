#include <darling/emulation/xnu_syscall/bsd/impl/xattr/fremovexattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/xattr_utils.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_fremovexattr(int fd, const char* name, int options)
{
	int ret;
	char linux_name[XATTR_LINUX_NAME_BUFSIZE];

	ret = xattr_name_to_linux(name, linux_name);
	if (ret < 0)
		return ret;

	ret = LINUX_SYSCALL(__NR_fremovexattr, fd, linux_name);

	if (ret < 0)
		return errno_linux_xattr_to_bsd(ret);

	return ret;
}
