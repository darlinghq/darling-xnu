#include <darling/emulation/xnu_syscall/bsd/impl/wrapped/sem_wait.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#include <elfcalls.h>

extern struct elf_calls* elfcalls(void);

long sys_sem_wait(int* sem)
{
	CANCELATION_POINT();
	return sys_sem_wait_nocancel(sem);
}

long sys_sem_wait_nocancel(int* sem)
{
#ifndef VARIANT_DYLD
	int ret;

	ret = elfcalls()->sem_wait(sem);
	if (ret == -1)
		ret = -errno_linux_to_bsd(elfcalls()->get_errno());

	return ret;
#else
	return -ENOSYS;
#endif
}
