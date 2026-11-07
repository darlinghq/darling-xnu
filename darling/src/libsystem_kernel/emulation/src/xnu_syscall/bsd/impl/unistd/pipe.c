#include <stdint.h>
#include <sys/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/pipe.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/nofile_limit.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>

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

	if (nofile_limit_exceeded(fd[0]) || nofile_limit_exceeded(fd[1])) {
		close_internal(fd[0]);
		close_internal(fd[1]);
		return -EMFILE;
	}

#if defined(__x86_64__)
	// The x86_64 __pipe stub stores %eax and %edx through %rdi after the
	// syscall, relying on %rdi being preserved as a kernel would; the code
	// above may have clobbered it.
	__asm__ __volatile__("movl %0, %%edx\n\tmovq %1, %%rdi" :: "m"(fd[1]), "m"(fd) : "edx", "rdi");
#elif defined(__i386__)
	__asm__ __volatile__("movl %0, %%edx" :: "m"(fd[1]) : "edx");
#elif defined(__aarch64__) || defined(__arm64__)
	__asm__ __volatile__("mov x1, %0" :: "r"((unsigned long)(unsigned int)fd[1]) : "x1");
#else
#	warning Missing assembly!
#endif
	return fd[0];
}
