#include <darling/emulation/xnu_syscall/bsd/impl/unistd/exit.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/elfcalls_wrapper.h>

long sys_exit(int status)
{
	int ret;

	native_exit(status);

	ret = LINUX_SYSCALL1(__NR_exit_group, status);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
