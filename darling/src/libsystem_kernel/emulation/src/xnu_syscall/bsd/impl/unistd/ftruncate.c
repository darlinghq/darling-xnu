#include <darling/emulation/xnu_syscall/bsd/impl/unistd/ftruncate.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_ftruncate(int fd, long long length)
{
	int ret;

#ifdef __NR_ftruncate64
	ret = LINUX_SYSCALL(__NR_ftruncate64, fd, LL_ARG(length));
#else
	ret = LINUX_SYSCALL(__NR_ftruncate, fd, LL_ARG(length));
#endif
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
