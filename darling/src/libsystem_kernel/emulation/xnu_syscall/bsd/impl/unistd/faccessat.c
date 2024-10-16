#include "faccessat.h"

#include <sys/errno.h>

#include <emulation/conversion/at.h>
#include <emulation/darling/server/vchroot_userspace.h>
#include <emulation/linux_api/linux_syscall.h>

extern char* strcpy(char* dst, const char* src);

long sys_faccessat(int fd, const char* filename, int amode, int flag)
{
	int ret;

	struct vchroot_expand_args vc;
	int linux_flags;

	vc.flags = (flag & BSD_AT_SYMLINK_NOFOLLOW) ? 0 : VCHROOT_FOLLOW;
	vc.dfd = atfd(fd);

	strcpy(vc.path, filename);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	linux_flags = atflags_bsd_to_linux(flag);
	if (linux_flags == LINUX_AT_INVALID)
		return -EINVAL;

	ret = LINUX_SYSCALL(__NR_faccessat, vc.dfd, vc.path, amode, linux_flags);

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
