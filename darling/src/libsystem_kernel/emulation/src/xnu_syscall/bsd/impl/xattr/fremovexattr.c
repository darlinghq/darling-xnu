#include <darling/emulation/xnu_syscall/bsd/impl/xattr/fremovexattr.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_fremovexattr(int fd, const char* name, int options)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_fremovexattr, fd, name);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}
