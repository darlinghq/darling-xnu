#include "sys/inotify.h"

#include <sys/errno.h>

#include <emulation/darling/server/vchroot_userspace.h>
#include <emulation/darling/tsd/perthread_wd.h>
#include <emulation/linux_api/linux_syscall.h>

extern long cerror(int err);

VISIBLE
int __linux_inotify_init(void)
{
	#if defined(__NR_inotify_init)
		int rv;

		rv = LINUX_SYSCALL(__NR_inotify_init);
		if (rv < 0)
		{
			cerror(errno_linux_to_bsd(-rv));
			return -1;
		}

		return rv;
	#else
		inotify_init1(0);
	#endif
}

VISIBLE
int __linux_inotify_init1(int flags)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_inotify_init1, flags);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}

VISIBLE
int __linux_inotify_add_watch(int fd, const char *name, uint32_t mask)
{
	int rv;
	struct vchroot_expand_args vc;

	if (!name)
		return -EFAULT;

	vc.flags = 0;
	vc.dfd = get_perthread_wd();

	strcpy(vc.path, name);
	rv = vchroot_expand(&vc);
	if (rv < 0)
		return errno_linux_to_bsd(rv);

	rv = LINUX_SYSCALL(__NR_inotify_add_watch, fd, vc.path, mask);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}
