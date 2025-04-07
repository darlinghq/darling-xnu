#include <darling/emulation/legacy_path/misc/getrusage.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_getrusage(int who, void* rusage)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_getrusage, who, rusage);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

