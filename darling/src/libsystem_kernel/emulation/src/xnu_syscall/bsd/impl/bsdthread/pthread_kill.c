#include <emulation/xnu_syscall/bsd/impl/bsdthread/pthread_kill.h>

#include <emulation/common/base.h>
#include <emulation/conversion/methods/errno.h>
#include <emulation/conversion/methods/signal.h>

#include <darlingserver/rpc.h>

long sys_pthread_kill(int thread_port, int sig)
{
	int ret = dserver_rpc_pthread_kill(thread_port, signum_bsd_to_linux(sig));
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

