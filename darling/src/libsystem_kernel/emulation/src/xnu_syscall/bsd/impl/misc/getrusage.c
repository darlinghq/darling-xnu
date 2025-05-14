#include <darling/emulation/xnu_syscall/bsd/impl/misc/getrusage.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_getrusage(int who, void* rusage)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_getrusage, who, rusage);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
