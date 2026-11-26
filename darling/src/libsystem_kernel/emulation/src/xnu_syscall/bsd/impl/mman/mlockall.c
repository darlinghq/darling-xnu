#include <darling/emulation/xnu_syscall/bsd/impl/mman/mlockall.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

// bsd/sys/mman.h. Linux uses the same two values.
#define MCL_CURRENT 0x0001
#define MCL_FUTURE  0x0002

long sys_mlockall(int how)
{
	long ret;

	if (how & ~(MCL_CURRENT | MCL_FUTURE))
		return -EINVAL;

	ret = LINUX_SYSCALL1(__NR_mlockall, how);
	return ret < 0 ? errno_linux_to_bsd(ret) : 0;
}

long sys_munlockall(int how)
{
	long ret;

	// Darwin's syscall takes a how argument, but munlockall always unlocks
	// every locked page; the argument has no meaning.
	(void) how;

	ret = LINUX_SYSCALL0(__NR_munlockall);
	return ret < 0 ? errno_linux_to_bsd(ret) : 0;
}
