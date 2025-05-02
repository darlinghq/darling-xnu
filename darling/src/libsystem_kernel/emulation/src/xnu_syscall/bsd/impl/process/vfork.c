#include <darling/emulation/xnu_syscall/bsd/impl/process/vfork.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/process/fork.h>

long sys_vfork(void)
{
	// this used to be a separate implementation, but it did everything exactly the same as sys_fork,
	// so now it just calls sys_fork to avoid duplicating code
	return sys_fork();
}
