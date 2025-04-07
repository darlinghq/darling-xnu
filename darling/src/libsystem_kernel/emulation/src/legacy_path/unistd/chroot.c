#include <darling/emulation/legacy_path/unistd/chroot.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_chroot(const char* path)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_chroot, path);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
