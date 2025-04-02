#include <emulation/xnu_syscall/bsd/impl/wrapped/shm_open.h>

#include <emulation/common/base.h>
#include <emulation/conversion/methods/errno.h>
#include <emulation/conversion/methods/fd/fcntl.h>
#include <emulation/linux_api/linux_syscall.h>

#include <elfcalls.h>

extern struct elf_calls* elfcalls(void);

long sys_shm_open(const char* name, int oflag, int mode)
{
#ifndef VARIANT_DYLD
	int ret;

	ret = elfcalls()->shm_open(name, oflags_bsd_to_linux(oflag), mode);
	if (ret == -1)
		ret = -errno_linux_to_bsd(elfcalls()->get_errno());

	return ret;
#else
	return -BSD_ENOSYS;
#endif
}
