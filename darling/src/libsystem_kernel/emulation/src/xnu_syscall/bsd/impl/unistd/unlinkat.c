#include <darling/emulation/xnu_syscall/bsd/impl/unistd/unlinkat.h>

#include <sys/errno.h>

#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>


extern char* strcpy(char* dst, const char* src);

long sys_unlinkat(int fd, const char* path, int flag)
{
	int ret;
	struct vchroot_expand_args vc;
	int linux_flags;

	vc.flags = 0;
	vc.dfd = atfd(fd);
	
	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	linux_flags = atflags_bsd_to_linux(flag);
	if (linux_flags == LINUX_AT_INVALID)
		return -EINVAL;

	ret = LINUX_SYSCALL(__NR_unlinkat, vc.dfd, vc.path, linux_flags);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
