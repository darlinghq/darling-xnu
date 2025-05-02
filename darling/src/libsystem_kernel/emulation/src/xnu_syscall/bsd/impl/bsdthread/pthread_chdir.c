#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_chdir.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

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
