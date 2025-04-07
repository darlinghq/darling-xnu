#include <darling/emulation/legacy_path/misc/getentropy.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/duct_errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

#define LINUX_GRND_RANDOM 2

long sys_getentropy(void* buf, unsigned long size)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_getrandom, buf, size, 0);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	if (ret < size)
		return -EIO;

	return 0;
}

