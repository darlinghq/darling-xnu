#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_mutexdrop.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/simple.h>

#include <darlingserver/rpc.h>

long sys_psynch_mutexdrop(void* mutex, uint32_t mgen, uint32_t ugen, uint64_t tid, uint32_t flags)
{
	uint32_t retval;
	int ret = dserver_rpc_psynch_mutexdrop(mutex, mgen, ugen, tid, flags, &retval);

	if (ret < 0) {
		__simple_printf("psynch_mutexdrop failed internally: %d", ret);
		__simple_abort();
	}

	return (ret) ? ret : retval;
}
