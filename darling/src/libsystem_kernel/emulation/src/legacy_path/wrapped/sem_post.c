#include "sem_post.h"
#include "../base.h"
#include "../errno.h"
#include <sys/errno.h>
#include <linux-syscalls/linux.h>
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
	return -ENOSYS;
#endif
}

