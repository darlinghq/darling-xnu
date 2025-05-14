#include <darling/emulation/xnu_syscall/bsd/impl/select/poll.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

struct ppoll_timespec {
    long    tv_sec;         /* seconds */
    long    tv_nsec;        /* nanoseconds */
};

long sys_poll(struct pollfd* fds, unsigned int nfds, int timeout)
{
	CANCELATION_POINT();
	return sys_poll_nocancel(fds, nfds, timeout);
}

long sys_poll_nocancel(struct pollfd* fds, unsigned int nfds, int timeout)
{
	int ret;

	#if defined(__NR_poll)
		ret = LINUX_SYSCALL(__NR_poll, fds, nfds, timeout);
	#else
		struct ppoll_timespec timeout_ts = {
			.tv_sec = (timeout % 1000) * 1000000,
			.tv_nsec = timeout / 1000
		};
		
		ret = LINUX_SYSCALL(__NR_ppoll, fds, nfds, (timeout < 0) ? NULL : &timeout_ts, NULL);
	#endif
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
