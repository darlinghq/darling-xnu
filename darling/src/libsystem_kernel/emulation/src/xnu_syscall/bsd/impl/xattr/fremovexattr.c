#include <darling/emulation/xnu_syscall/bsd/impl/xattr/fremovexattr.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/xattr_utils.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_fremovexattr(int fd, const char* name, int options)
{
	int ret;
	char l_name[XATTR_NAME_MAX_LEN];

	const char* linux_name = xattr_name_to_linux(name, l_name, sizeof(l_name));

	ret = LINUX_SYSCALL(__NR_fremovexattr, fd, linux_name);

	if (ret < 0)
		return errno_linux_xattr_to_bsd(ret);

	return ret;
}
