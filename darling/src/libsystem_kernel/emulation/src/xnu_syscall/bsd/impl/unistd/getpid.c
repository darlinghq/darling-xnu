#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getpid.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_getpid(void)
{
	int ret;

	ret = LINUX_SYSCALL0(__NR_getpid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
