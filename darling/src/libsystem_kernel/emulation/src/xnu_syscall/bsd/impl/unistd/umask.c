#include <darling/emulation/xnu_syscall/bsd/impl/unistd/umask.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_umask(unsigned int newmask)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_umask, newmask);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
