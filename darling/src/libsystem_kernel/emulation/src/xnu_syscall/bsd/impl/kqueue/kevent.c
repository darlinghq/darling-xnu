#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kevent.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

int WEAK_IMPORT kevent_impl(int kq, ...);

long sys_kevent(int	kq, const struct kevent	*changelist, int nchanges,
			struct	kevent *eventlist, int nevents,
			const struct timespec *timeout)
{
	if (!kevent_impl)
		return -ENOSYS;

	int ret = kevent_impl(kq, changelist, nchanges, eventlist, nevents, timeout);
	if (ret < 0)
		ret = -errno;
	return ret;
}
