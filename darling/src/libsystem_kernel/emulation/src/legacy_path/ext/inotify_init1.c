#include <darling/emulation/legacy_path/ext/sys/inotify.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

extern long cerror(int __err);

VISIBLE
int inotify_init1 (int __flags)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_inotify_init1, __flags);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}

