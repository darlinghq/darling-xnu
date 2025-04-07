#include <darling/emulation/legacy_path/xattr/fremovexattr.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_fremovexattr(int fd, const char* name, int options)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_fremovexattr, fd, name);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}

