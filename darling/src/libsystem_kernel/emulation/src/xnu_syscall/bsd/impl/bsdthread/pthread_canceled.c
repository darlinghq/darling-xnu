#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/tsd.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/bsdthread_register.h>

#include <darlingserver/rpc.h>

static bool cancel_marked(void)
{
	// darlingserver sets this TSD slot when the thread is marked for cancellation.
	// before libpthread has registered (e.g. in dyld), there may be no TSD to read.
	return bsdthread_registered() && _pthread_getspecific_direct(__PTK_DARLING_CANCEL_MARKED);
}

long sys_pthread_canceled(int action)
{
	if (action == 0 && !cancel_marked())
		return -EINVAL;

	int ret = dserver_rpc_pthread_canceled(action);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else if (action == 0)
	{
		// the mark has been acted on and darlingserver won't mark this thread again,
		// so the cancellation points in its cleanup handlers can skip the call
		_pthread_setspecific_direct(__PTK_DARLING_CANCEL_MARKED, 0);
	}

	return ret;
}

// unlike sys_pthread_canceled(0), this leaves the thread marked,
// so that the EINTR handling in libc can still act on it
bool pthread_testcancel_presyscall(void)
{
	return cancel_marked() && dserver_rpc_pthread_testcancel() == 0;
}
