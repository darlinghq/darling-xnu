#include <darling/emulation/legacy_path/mach/lkm.h>
#include <darling/emulation/legacy_path/unistd/readlink.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/common_at.h>
#include <darling/emulation/legacy_path/vchroot_expand.h>

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
