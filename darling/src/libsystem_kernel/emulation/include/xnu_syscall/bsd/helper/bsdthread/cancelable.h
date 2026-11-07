#ifndef _BSDTHREAD_CANCELLABLE_H
#define _BSDTHREAD_CANCELLABLE_H

#include <sys/errno.h>

#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>

#define CANCELATION_POINT() \
	if (pthread_testcancel_presyscall()) \
		return -EINTR;

#endif // _BSDTHREAD_CANCELLABLE_H
