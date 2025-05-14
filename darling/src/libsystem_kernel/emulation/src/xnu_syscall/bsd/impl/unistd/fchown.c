#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchown.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_fchown(int fd, int uid, int gid)
{
#if 0
	int ret;

#ifdef __NR_fchown32
	ret = LINUX_SYSCALL3(__NR_fchown32, fd, uid, gid);
#else
	ret = LINUX_SYSCALL3(__NR_fchown, fd, uid, gid);
#endif
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
#else
	return 0;
#endif
}
