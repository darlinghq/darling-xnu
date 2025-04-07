#include <darling/emulation/legacy_path/psynch/psynch_cvwait.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/simple.h>
#include <darling/emulation/legacy_path/duct_errno.h>

#include <darlingserver/rpc.h>

long sys_psynch_cvwait(void* cv, uint64_t cvlsgen, uint32_t cvugen, void * mutex, uint64_t mugen, 
		uint32_t flags, int64_t sec, uint32_t nsec)
{
	uint32_t retval;
	int ret = dserver_rpc_psynch_cvwait(cv, cvlsgen, cvugen, mutex, mugen, flags, sec, nsec, &retval);

	if (ret < 0) {
		if (ret == -LINUX_EINTR) {
			return -EINTR;
		}
		__simple_printf("psynch_cvwait failed internally: %d", ret);
		__simple_abort();
	}

	return (ret) ? ret : retval;
}

