#include <darling/emulation/xnu_syscall/bsd/impl/unistd/mknodat.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/other/mach/lkm.h>

extern char* strcpy(char* dst, const char* src);

long sys_mknodat(int fd, const char* path, int mode, int dev)
{
	int ret;
	struct vchroot_expand_args vc;

	if (!path)
		return -EFAULT;

	vc.flags = 0;
	vc.dfd = atfd(fd);

	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	ret = LINUX_SYSCALL(__NR_mknodat, vc.dfd, vc.path, mode, dev);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return 0;
}
