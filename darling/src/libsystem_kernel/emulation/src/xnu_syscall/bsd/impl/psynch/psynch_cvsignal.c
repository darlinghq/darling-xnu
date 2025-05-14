#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_cvsignal.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/simple.h>

#include <darlingserver/rpc.h>

long sys_psynch_cvsignal(void* cv, uint32_t cvlsgen, uint32_t cvugen, int thread_port, void* mutex, uint32_t mugen,
		uint64_t tid, uint32_t flags)
{
	uint32_t retval;
	int ret = dserver_rpc_psynch_cvsignal(cv, cvlsgen, cvugen, thread_port, mutex, mugen, tid, flags, &retval);

	if (ret < 0) {
		__simple_printf("psynch_cvsignal failed internally: %d", ret);
		__simple_abort();
	}

	return (ret) ? ret : retval;
}
