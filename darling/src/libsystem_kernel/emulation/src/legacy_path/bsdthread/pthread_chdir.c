#include <darling/emulation/legacy_path/bsdthread/pthread_chdir.h>

#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/mach/lkm.h>
#include <darling/emulation/legacy_path/fcntl/open.h>
#include <darling/emulation/legacy_path/unistd/close.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

long sys_pthread_chdir(const char* path)
{
	int rv, newfd, oldfd;

	newfd = sys_open(path, BSD_O_RDONLY | BSD_O_DIRECTORY | BSD_O_CLOEXEC, 0);
	if (newfd < 0)
		return newfd;

	oldfd = get_perthread_wd();
	if (oldfd != LINUX_AT_FDCWD)
		close_internal(oldfd);

	set_perthread_wd(newfd);
	return 0;
}

