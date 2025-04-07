#include <darling/emulation/legacy_path/unistd/dup.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

__attribute__((weak))
__attribute__((visibility("default")))
void kqueue_dup(int oldfd, int newfd) { }

long sys_dup(int fd)
{
	int ret;

	ret = LINUX_SYSCALL1(__NR_dup, fd);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else
		kqueue_dup(fd, ret);

	return ret;
}

