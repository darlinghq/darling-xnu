#include <emulation/xnu_syscall/bsd/impl/bsdthread/disable_threadsignal.h>

#include <stddef.h>

#include <emulation/common/base.h>
#include <emulation/conversion/methods/errno.h>
#include <emulation/conversion/methods/signal.h>
#include <emulation/linux_api/linux_syscall.h>
#include <emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>
#include <emulation/xnu_syscall/bsd/impl/signal/sigprocmask.h>

long sys_disable_threadsignal(int disable)
{
	if (!disable)
		return -BSD_ENOTSUP;

	bsd_sigset_t set = ~0;
	
	// Disable cancelation
	sys_pthread_canceled(2);

	// Signal config is per-thread on Linux
	return sys_sigprocmask(BSD_SIG_BLOCK, &set, NULL);
}

