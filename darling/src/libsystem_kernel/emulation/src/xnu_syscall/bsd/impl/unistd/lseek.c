#include <darling/emulation/xnu_syscall/bsd/impl/unistd/lseek.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long long sys_lseek(int fd, long long offset, int whence)
{
	long long ret;

#ifdef __NR__llseek
	// The plain lseek takes and returns a 32-bit off_t on 32-bit Linux;
	// _llseek takes the offset as two halves and stores the 64-bit result.
	long long roff;
	ret = LINUX_SYSCALL(__NR__llseek, fd, (unsigned int)((unsigned long long)offset >> 32),
			(unsigned int)offset, &roff, whence);

	if (ret >= 0)
		ret = roff;
#else
	ret = LINUX_SYSCALL(__NR_lseek, fd, offset, whence);
#endif
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
