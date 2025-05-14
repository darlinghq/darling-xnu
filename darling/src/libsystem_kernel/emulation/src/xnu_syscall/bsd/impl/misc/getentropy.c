#include <darling/emulation/xnu_syscall/bsd/impl/misc/getentropy.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

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
