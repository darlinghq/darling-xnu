#include <emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>

#include <darlingserver/rpc.h>

#include <emulation/conversion/methods/errno.h>
#include <emulation/linux_api/linux_syscall.h>
#include <emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_create.h>

long sys_pthread_canceled(int action)
{
	if (action == 0 && !uses_threads())
		return -BSD_EINVAL;

	int ret = dserver_rpc_pthread_canceled(action);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

