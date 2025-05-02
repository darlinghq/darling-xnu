#include <darling/emulation/xnu_syscall/bsd/impl/wrapped/shm_unlink.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#include <elfcalls.h>

extern struct elf_calls* elfcalls(void);

long sys_shm_unlink(const char* name)
{
#ifndef VARIANT_DYLD
	int ret;

	ret = elfcalls()->shm_unlink(name);
	if (ret == -1)
		ret = -errno_linux_to_bsd(elfcalls()->get_errno());

	return ret;
#else
	return -ENOSYS;
#endif
}
