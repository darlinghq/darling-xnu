#include "sys/signalfd.h"

#include <emulation/conversion/signal.h>
#include <emulation/linux_api/linux_syscall.h>
#include <emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>

extern long cerror(int __err);

VISIBLE
int __linux_signalfd(int fd, const sigset_t *mask, int flags)
{
	int rv, linux_flags = 0;
	linux_sigset_t linux_mask;

	sigset_bsd_to_linux(mask, &linux_mask);

	if (flags & O_CLOEXEC)
		linux_flags |= 02000000;
	if (flags & O_NONBLOCK)
		linux_flags |= 04000;

#ifdef __NR_signalfd4
	rv = LINUX_SYSCALL(__NR_signalfd4, fd, &linux_mask, 8, linux_flags);
#else
	rv = LINUX_SYSCALL(__NR_signalfd, fd, &linux_mask, 8);
	// handle flags
	if (rv >= 0)
	{
		if (flags & O_CLOEXEC)
			sys_fcntl(rv, F_SETFD, BSD_FD_CLOEXEC);
		if (flags & O_NONBLOCK)
			sys_fcntl(rv, F_SETFL, BSD_O_NONBLOCK);
	}
#endif
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}

