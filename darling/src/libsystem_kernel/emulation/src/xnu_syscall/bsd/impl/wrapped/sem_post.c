#include <emulation/xnu_syscall/bsd/impl/wrapped/sem_post.h>

#include <emulation/common/base.h>
#include <emulation/conversion/methods/errno.h>
#include <emulation/conversion/methods/fd/fcntl.h>
#include <emulation/linux_api/linux_syscall.h>

#include <elfcalls.h>

extern struct elf_calls* elfcalls(void);

long sys_sem_post(int* sem)
{
#ifndef VARIANT_DYLD
	int ret;

	// __simple_printf("sem_post(%p)\n", sem);
	ret = elfcalls()->sem_post(sem);
	if (ret == -1)
		ret = -errno_linux_to_bsd(elfcalls()->get_errno());

	return ret;
#else
	return -BSD_ENOSYS;
#endif
}
