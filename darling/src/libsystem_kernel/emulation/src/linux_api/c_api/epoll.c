#include <emulation/linux_api/c_api/sys/epoll.h>

#include <emulation/linux_api/linux_syscall.h>

extern long cerror(int err);

VISIBLE
int __linux_epoll_create(int size)
{
	#if defined(__NR_epoll_create)
		int rv;

		rv = LINUX_SYSCALL(__NR_epoll_create, size);
		if (rv < 0)
		{
			cerror(errno_linux_to_bsd(-rv));
			return -1;
		}

		return rv;
	#else
		// The size argument in __NR_epoll_create is ignored
		return __linux_epoll_create1(0);
	#endif
}

VISIBLE
int __linux_epoll_create1(int flags)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_epoll_create1, flags);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}

VISIBLE
int __linux_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_epoll_ctl, epfd, op, fd, event);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}

VISIBLE
int __linux_epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
	int rv;

	#if defined(__NR_epoll_wait)
		rv = LINUX_SYSCALL(__NR_epoll_wait, epfd, events, maxevents, timeout);
	#else
		rv = LINUX_SYSCALL(__NR_epoll_pwait, epfd, events, maxevents, timeout, NULL);
	#endif

	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}
