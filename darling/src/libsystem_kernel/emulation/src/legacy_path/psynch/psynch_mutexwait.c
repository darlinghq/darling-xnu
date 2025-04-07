#include <darling/emulation/legacy_path/psynch/psynch_mutexwait.h>


#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/simple.h>
#include <darling/emulation/legacy_path/duct_errno.h>

#include <darlingserver/rpc.h>

long sys_psynch_mutexwait(void* mutex, uint32_t mgen, uint32_t ugen, uint64_t tid, uint32_t flags)
{
	uint32_t retval;
	int ret = dserver_rpc_psynch_mutexwait(mutex, mgen, ugen, tid, flags, &retval);

	if (ret < 0) {
		if (ret == -LINUX_EINTR) {
			return -EINTR;
		}
		__simple_printf("psynch_mutexwait failed internally: %d", ret);
		__simple_abort();
	}

	return (ret) ? ret : retval;
}

