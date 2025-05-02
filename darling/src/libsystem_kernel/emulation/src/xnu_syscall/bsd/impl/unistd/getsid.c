#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getsid.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_getsid(int pid)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_getsid, pid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
