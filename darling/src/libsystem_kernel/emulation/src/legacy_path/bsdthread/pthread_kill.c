#include <darling/emulation/legacy_path/bsdthread/pthread_kill.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/signal/duct_signals.h>
#include <darling/emulation/legacy_path/simple.h>

#include <darlingserver/rpc.h>

long sys_pthread_kill(int thread_port, int sig)
{
	int ret = dserver_rpc_pthread_kill(thread_port, signum_bsd_to_linux(sig));
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

