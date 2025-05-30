#include <darling/emulation/xnu_syscall/bsd/impl/unistd/pipe.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_pipe(int fd[2])
{
	int ret;

	#if defined(__NR_pipe)
		ret = LINUX_SYSCALL(__NR_pipe, fd);
	#else
		ret = LINUX_SYSCALL(__NR_pipe2, fd, 0);
	#endif
	if (ret < 0)
		return errno_linux_to_bsd(ret);

#if defined(__i386__) || defined(__x86_64__)
	__asm__ __volatile__("movl %0, %%edx" :: "m"(fd[1]) : "edx");
#else
#	warning Missing assembly!
#endif
	return fd[0];
}
