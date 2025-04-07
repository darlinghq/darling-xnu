#include <darling/emulation/legacy_path/xattr/fsetxattr.h>

#include <sys/errno.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_fsetxattr(int fd, const char* name, const void* value,
		unsigned long size, unsigned int pos, int options)
{
	int ret;

	if (pos != 0)
		return -ERANGE;

	ret = LINUX_SYSCALL(__NR_fsetxattr, fd, name, value, size);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}

