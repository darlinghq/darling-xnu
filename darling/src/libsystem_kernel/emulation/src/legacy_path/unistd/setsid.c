#include <darling/emulation/legacy_path/unistd/setsid.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_setsid(void)
{
	int ret;

	ret = LINUX_SYSCALL0(__NR_setsid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

