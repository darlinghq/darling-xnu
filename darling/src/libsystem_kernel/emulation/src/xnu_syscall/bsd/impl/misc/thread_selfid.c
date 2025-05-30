#include <darling/emulation/xnu_syscall/bsd/impl/misc/thread_selfid.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

VISIBLE // made visible for xtrace
long sys_thread_selfid(void)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_gettid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
