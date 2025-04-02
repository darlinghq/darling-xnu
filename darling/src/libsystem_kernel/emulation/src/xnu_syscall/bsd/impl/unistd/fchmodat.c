#include <emulation/xnu_syscall/bsd/impl/unistd/fchmodat.h>

#include <emulation/conversion/methods/at.h>
#include <emulation/conversion/methods/errno.h>
#include <emulation/darling/server/vchroot_userspace.h>
#include <emulation/xnu_syscall/shared/at.h>

#include <stdio.h>
#include <sys/stat.h>
#include <sys/errno.h>

extern char* strcpy(char* dst, const char* src);
extern long close_internal(int fd);

extern int do_linux_fchmodat(int fd, const char* path, int mode, int flag);

long sys_fchmodat(int fd, const char* path, int mode, int flag)
{
	int ret;
	struct vchroot_expand_args vc;
	int linux_flags;

	vc.flags = (flag & BSD_AT_SYMLINK_NOFOLLOW) ? 0 : VCHROOT_FOLLOW;
	vc.dfd = atfd(fd);

	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	linux_flags = atflags_bsd_to_linux(flag);
	if (linux_flags == LINUX_AT_INVALID)
		return -EINVAL;

	ret = do_linux_fchmodat(vc.dfd, vc.path, mode, linux_flags);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
