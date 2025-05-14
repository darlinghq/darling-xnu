#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_fchdir.h>

#include <stddef.h>
#include <sys/fcntl.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/dup.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

long sys_pthread_fchdir(int newfd)
{
	int fd = get_perthread_wd();

	if (fd != LINUX_AT_FDCWD)
		close_internal(fd);

	if (newfd == -1)
		newfd = LINUX_AT_FDCWD; // return to per-process wd
	else
	{
		newfd = sys_dup(newfd);
		if (newfd < 0)
			return newfd;
		sys_fcntl(newfd, F_SETFD, FD_CLOEXEC);
	}

	set_perthread_wd(newfd);
	return 0;
}
