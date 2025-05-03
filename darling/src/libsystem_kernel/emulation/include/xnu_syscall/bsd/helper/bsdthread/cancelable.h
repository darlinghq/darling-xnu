#ifndef _BSDTHREAD_CANCELLABLE_H
#define _BSDTHREAD_CANCELLABLE_H

#include <sys/errno.h>

#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>

#define CANCELATION_POINT() \
	if (sys_pthread_canceled(0) == 0) \
		return -EINTR;

#endif // _BSDTHREAD_CANCELLABLE_H
