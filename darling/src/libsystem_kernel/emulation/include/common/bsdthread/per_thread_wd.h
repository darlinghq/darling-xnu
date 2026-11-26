#ifndef _PER_THREAD_WD_H
#define _PER_THREAD_WD_H

#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/common/tsd.h>

// We use the static TSD key __PTK_DARLING_PERTHREAD_WD_FD (see `../tsd.h` and `pthread/tsd_private.h`).
// It holds the per thread wd file descriptor minus LINUX_AT_FDCWD, so that its initial value (0) means
// "not set" (any descriptor, 0 included, is stored as a nonzero value).
//
// NOTE: The file descriptor in __PTK_DARLING_PERTHREAD_WD_FD will get leaked unless __pthread_fchdir(-1)
// is called, but this is such a rare scenario that we disregard it.
// It is be too hard to fix, because static TSD keys (such as the ones we use)
// don't have destructors.

static inline int get_perthread_wd(void)
{
	return (int) (intptr_t) _pthread_getspecific_direct(__PTK_DARLING_PERTHREAD_WD_FD) + LINUX_AT_FDCWD;
}

static inline void set_perthread_wd(int fd)
{
	_pthread_setspecific_direct(__PTK_DARLING_PERTHREAD_WD_FD, (void*) ((intptr_t) fd - LINUX_AT_FDCWD));
}

#endif // _PER_THREAD_WD_H
