#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_markcancel.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#include <darlingserver/rpc.h>

long sys_pthread_markcancel(unsigned int thread_port)
{
	int ret = dserver_rpc_pthread_markcancel(thread_port);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
