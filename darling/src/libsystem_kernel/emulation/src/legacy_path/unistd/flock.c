#include <darling/emulation/legacy_path/unistd/flock.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_flock(int fd, int op)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_flock, fd, op);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
