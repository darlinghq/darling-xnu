#include <sys/errno.h>

#include <darling/emulation/linux_premigration/ext/sys/inotify.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern long cerror(int __err);

VISIBLE
int inotify_add_watch (int __fd, const char *__name, uint32_t __mask)
{
	int rv;
	struct vchroot_expand_args vc;

	if (!__name)
		return -EFAULT;

	vc.flags = 0;
	vc.dfd = get_perthread_wd();

	strcpy(vc.path, __name);
	rv = vchroot_expand(&vc);

	if (rv < 0)
		return errno_linux_to_bsd(rv);

	rv = LINUX_SYSCALL(__NR_inotify_add_watch, __fd, vc.path, __mask);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}
