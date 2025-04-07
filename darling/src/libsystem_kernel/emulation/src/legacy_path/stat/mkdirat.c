#include <darling/emulation/legacy_path/stat/mkdirat.h>

#include <sys/errno.h>

#include <darling/emulation/legacy_path/stat/common.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/common_at.h>
#include <darling/emulation/legacy_path/vchroot_expand.h>
#include <darling/emulation/legacy_path/mach/lkm.h>

extern char* strcpy(char* dst, const char* src);

long sys_mkdirat(int fd, const char* path, unsigned int mode)
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

	ret = LINUX_SYSCALL(__NR_mkdirat, vc.dfd, vc.path, mode);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return 0;
}
