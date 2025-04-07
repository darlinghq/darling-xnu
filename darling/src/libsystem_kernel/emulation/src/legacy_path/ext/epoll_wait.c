#include <string.h>

#include <darling/emulation/legacy_path/ext/sys/epoll.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

extern long cerror(int __err);

VISIBLE
int epoll_wait (int __epfd, struct epoll_event *__events,
		int __maxevents, int __timeout)
{
	int rv;

	#if defined(__NR_epoll_wait)
		rv = LINUX_SYSCALL(__NR_epoll_wait, __epfd, __events, __maxevents,
			__timeout);
	#else
		rv = LINUX_SYSCALL(__NR_epoll_pwait, __epfd, __events, __maxevents,
			__timeout, NULL);
	#endif
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}

