#include <darling/emulation/xnu_syscall/bsd/impl/unistd/setsid.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_setsid(void)
{
	int ret;

	ret = LINUX_SYSCALL0(__NR_setsid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
