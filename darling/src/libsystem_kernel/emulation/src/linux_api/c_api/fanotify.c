#include <emulation/linux_api/c_api/fanotify.h>

#include <emulation/conversion/methods/errno.h>
#include <emulation/darling/server/vchroot_userspace.h>
#include <emulation/linux_api/linux_syscall.h>

extern long cerror(int __err);

extern char* strcpy(char* dst, const char* src);

int linux_fanotify_init(unsigned flags, unsigned event_f_flags)
{
	int rv = LINUX_SYSCALL(__NR_fanotify_init, flags, event_f_flags);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}
	return rv;
}

int linux_fanotify_mark(int fanotify_fd, unsigned flags, unsigned long long mask, int dfd, const char *pathname)
{
	struct vchroot_expand_args vc;

	if (!pathname)
		return -BSD_EFAULT;

	vc.flags = 0;
	vc.dfd = dfd;

	strcpy(vc.path, pathname);
	int rv = vchroot_expand(&vc);

	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	rv = LINUX_SYSCALL(__NR_fanotify_mark, fanotify_fd, flags, mask, dfd, vc.path);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}
	return rv;
}
