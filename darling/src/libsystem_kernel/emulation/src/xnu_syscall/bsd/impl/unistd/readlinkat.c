#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readlinkat.h>

#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readlink.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>

extern __SIZE_TYPE__ strlen(const char* str);
extern char* strcpy(char* dst, const char* src);

long sys_readlinkat(int fd, const char* path, char* buf, int count)
{
	int ret;
	struct vchroot_expand_args vc;

	vc.flags = 0;
	vc.dfd = atfd(fd);
	
	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	ret = LINUX_SYSCALL(__NR_readlinkat, vc.dfd, vc.path, buf, count);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
