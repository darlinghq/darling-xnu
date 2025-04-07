#include <darling/emulation/legacy_path/unistd/sync.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_sync(void)
{
	int ret;

	ret = LINUX_SYSCALL0(__NR_sync);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

