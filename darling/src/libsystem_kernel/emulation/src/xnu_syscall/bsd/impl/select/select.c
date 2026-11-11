#include <darling/emulation/xnu_syscall/bsd/impl/select/select.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/xnu_syscall/bsd/impl/select/pselect.h>

long sys_select(int nfds, void* rfds, void* wfds, void* efds, struct bsd_timeval* timeout)
{
	CANCELATION_POINT();
	return sys_select_nocancel(nfds, rfds, wfds, efds, timeout);
}

long sys_select_nocancel(int nfds, void* rfds, void* wfds, void* efds, struct bsd_timeval* timeout)
{
	// Go through pselect, which reports the exceptional conditions of
	// pseudo-terminal masters the way macOS does; pselect() takes a
	// timespec, not a timeval.
	struct bsd_timespec ts;

	if (timeout != NULL)
	{
		ts.tv_sec = timeout->tv_sec;
		ts.tv_nsec = (long)timeout->tv_usec * 1000L;
	}

	return sys_pselect_nocancel(nfds, rfds, wfds, efds,
			(timeout != NULL) ? &ts : NULL, NULL);
}
