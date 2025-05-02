#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kevent64.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

int __attribute__((weak)) __attribute__((visibility("default"))) kevent64_impl(int kq, ...) { return -ENOSYS; }

long sys_kevent64(int kq, const struct kevent64_s *changelist, int nchanges,
			struct kevent64_s *eventlist, int nevents, unsigned int flags,
			const struct timespec *timeout)
{
	int ret = kevent64_impl(kq, changelist, nchanges, eventlist, nevents, flags, timeout);
	if (ret < 0)
		ret = -errno;
	return ret;
}
