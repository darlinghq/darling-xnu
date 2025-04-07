#include <darling/emulation/legacy_path/process/vfork.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/signal/duct_signals.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/process/fork.h>

long sys_vfork(void)
{
	// this used to be a separate implementation, but it did everything exactly the same as sys_fork,
	// so now it just calls sys_fork to avoid duplicating code
	return sys_fork();
}

