#ifndef DARLING_EMULATION_SHARED_CANCELLABLE_H
#define DARLING_EMULATION_SHARED_CANCELLABLE_H

#include <emulation/conversion/methods/errno.h>
#include <emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>

// TODO: I think this macro should belong in "pthread_canceled.h" instead

#define CANCELATION_POINT() \
	if (sys_pthread_canceled(0) == 0) \
		return -BSD_EINTR;

#endif // DARLING_EMULATION_SHARED_CANCELLABLE_H
