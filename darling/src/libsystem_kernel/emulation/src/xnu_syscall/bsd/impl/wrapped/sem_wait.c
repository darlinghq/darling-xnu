#include <emulation/xnu_syscall/bsd/impl/wrapped/sem_wait.h>

#include <emulation/common/base.h>
#include <emulation/conversion/methods/errno.h>
#include <emulation/conversion/methods/fd/fcntl.h>
#include <emulation/linux_api/linux_syscall.h>
#include <emulation/xnu_syscall/shared/bsdthread/cancelable.h>

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
	return -BSD_ENOSYS;
#endif
}
