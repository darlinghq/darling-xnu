#ifndef _PER_THREAD_WD_H
#define _PER_THREAD_WD_H
#include "../common_at.h"
#include "../tsd.h"

// We use the following static TSD keys (see `../tsd.h` and `pthread/tsd_private.h`):
// __PTK_DARLING_PERTHREAD_WD_IS_SET = is __PTK_DARLING_PERTHREAD_WD_FD set? (Because 0 could be a valid FD, theoretically.)
// __PTK_DARLING_PERTHREAD_WD_FD = per thread wd file descriptor
//
// NOTE: The file descriptor in __PTK_DARLING_PERTHREAD_WD_FD will get leaked unless __pthread_fchdir(-1)
// is called, but this is such a rare scenario that we disregard it.
// It is be too hard to fix, because static TSD keys (such as the ones we use)
// don't have destructors.

static inline int get_perthread_wd(void)
{
	if (_pthread_getspecific_direct(__PTK_DARLING_PERTHREAD_WD_IS_SET))
		return _pthread_getspecific_direct(__PTK_DARLING_PERTHREAD_WD_FD);
	else
		return LINUX_AT_FDCWD;
}

static inline void set_perthread_wd(int fd)
{
	_pthread_setspecific_direct(__PTK_DARLING_PERTHREAD_WD_IS_SET, 1);
	_pthread_setspecific_direct(__PTK_DARLING_PERTHREAD_WD_FD, fd);
}

#endif

