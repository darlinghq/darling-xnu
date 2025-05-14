#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kqueue.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/simple.h>

int __attribute__((weak)) __attribute__((visibility("default"))) kqueue_impl(void)
{
	__simple_printf("No kqueue implementation?!\n");
	return -ENOSYS;
}

long sys_kqueue(void)
{
	int ret = kqueue_impl();
	if (ret < 0)
		ret = -errno;
	return ret;
}
