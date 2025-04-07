#include <darling/emulation/legacy_path/wrapped/sem_unlink.h>

#include <sys/errno.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

#include <elfcalls.h>

extern struct elf_calls* elfcalls(void);

long sys_sem_unlink(const char* name)
{
#ifndef VARIANT_DYLD
	int ret;

	ret = elfcalls()->sem_unlink(name);
	if (ret == -1)
		ret = -errno_linux_to_bsd(elfcalls()->get_errno());

	return ret;
#else
	return -ENOSYS;
#endif
}

