#include <darling/emulation/legacy_path/bsdthread/pthread_fchdir.h>

#include <stddef.h>
#include <sys/fcntl.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/mach/lkm.h>
#include <darling/emulation/legacy_path/fcntl/open.h>
#include <darling/emulation/legacy_path/fcntl/fcntl.h>
#include <darling/emulation/legacy_path/unistd/close.h>
#include <darling/emulation/legacy_path/unistd/dup.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

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

