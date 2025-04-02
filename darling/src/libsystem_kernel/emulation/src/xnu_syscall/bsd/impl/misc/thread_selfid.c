#include <emulation/xnu_syscall/bsd/impl/misc/thread_selfid.h>

#include <emulation/linux_api/linux_syscall.h>

VISIBLE // made visible for xtrace
long sys_thread_selfid(void)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_gettid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
