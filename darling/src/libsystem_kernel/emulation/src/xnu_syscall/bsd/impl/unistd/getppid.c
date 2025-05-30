#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getppid.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_getppid(void)
{
	int ret;

	ret = LINUX_SYSCALL0(__NR_getppid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	// The PID 1 process in a Linux PID namespace has PID 1, but PPID 0.
	// This fixes the PPID value to 1 (which is what a real PID 1 process gets).
	// Apple's launchd requires this.
	if (ret == 0)
		ret = 1;

	return ret;
}
