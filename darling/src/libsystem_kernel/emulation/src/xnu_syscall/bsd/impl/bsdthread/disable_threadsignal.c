#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/disable_threadsignal.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigprocmask.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>

#ifndef SIG_BLOCK
#	define SIG_BLOCK	1
#endif

long sys_disable_threadsignal(int disable)
{
	if (!disable)
		return -ENOTSUP;

	sigset_t set = ~0;
	
	// Disable cancelation
	sys_pthread_canceled(2);

	// Signal config is per-thread on Linux
	return sys_sigprocmask(SIG_BLOCK, &set, NULL);
}
