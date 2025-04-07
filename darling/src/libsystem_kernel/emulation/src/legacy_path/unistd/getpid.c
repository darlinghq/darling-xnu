#include <darling/emulation/legacy_path/unistd/getpid.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_getpid(void)
{
	int ret;

	ret = LINUX_SYSCALL0(__NR_getpid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

