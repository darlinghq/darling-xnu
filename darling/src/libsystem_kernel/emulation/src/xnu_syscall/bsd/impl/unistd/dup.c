#include <darling/emulation/xnu_syscall/bsd/impl/unistd/dup.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/nofile_limit.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bpf/bpf_device.h>

__attribute__((weak))
__attribute__((visibility("default")))
void kqueue_dup(int oldfd, int newfd) { }

long sys_dup(int fd)
{
	int ret;

	ret = LINUX_SYSCALL1(__NR_dup, fd);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else {
		ret = nofile_limit_check_new(ret);
		if (ret >= 0) {
			kqueue_dup(fd, ret);
			bpf_device_dup(fd, ret);
		}
	}

	return ret;
}
