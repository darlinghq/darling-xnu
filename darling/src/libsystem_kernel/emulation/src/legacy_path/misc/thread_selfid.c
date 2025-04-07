#include <darling/emulation/legacy_path/misc/thread_selfid.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

VISIBLE // made visible for xtrace
long sys_thread_selfid(void)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_gettid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

