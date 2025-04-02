#include "disable_threadsignal.h"
#include "../base.h"
#include "../errno.h"
#include "../signal/duct_signals.h"
#include "../signal/sigprocmask.h"
#include <sys/errno.h>
#include <linux-syscalls/linux.h>
#include <stddef.h>
#include "pthread_canceled.h"

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

