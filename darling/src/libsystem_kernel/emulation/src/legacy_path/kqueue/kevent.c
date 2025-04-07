#include <darling/emulation/legacy_path/kqueue/kevent.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

int __attribute__((weak)) __attribute__((visibility("default"))) kevent_impl(int kq, ...) { return -ENOSYS; }

long sys_kevent(int	kq, const struct kevent	*changelist, int nchanges,
			struct	kevent *eventlist, int nevents,
			const struct timespec *timeout)
{
	int ret = kevent_impl(kq, changelist, nchanges, eventlist, nevents, timeout);
	if (ret < 0)
		ret = -errno;
	return ret;
}

