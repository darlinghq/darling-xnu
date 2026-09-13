#include <darling/emulation/xnu_syscall/bsd/impl/unistd/mknodat.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/other/mach/lkm.h>

extern char* strcpy(char* dst, const char* src);

static inline unsigned long long dev_bsd_to_linux(int dev)
{
	if (dev == 0)
		return 0;
	unsigned int maj = ((unsigned int)dev >> 24) & 0xff;
	unsigned int min = (unsigned int)dev & 0xffffff;
	return ((unsigned long long)(maj & 0xfff) << 8) | (min & 0xff) |
	       (((unsigned long long)(maj & ~0xfff)) << 32) |
	       (((unsigned long long)(min & ~0xff)) << 12);
}

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

	ret = LINUX_SYSCALL(__NR_mknodat, vc.dfd, vc.path, mode, dev_bsd_to_linux(dev));

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return 0;
}
