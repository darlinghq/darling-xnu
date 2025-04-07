#include <darling/emulation/legacy_path/unistd/setpgid.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_setpgid(int pid, int pgid)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_setpgid, pid, pgid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

