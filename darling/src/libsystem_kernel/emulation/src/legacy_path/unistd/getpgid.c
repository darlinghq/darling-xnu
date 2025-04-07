#include <darling/emulation/legacy_path/unistd/getpgid.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_getpgid(int pid)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_getpgid, pid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

