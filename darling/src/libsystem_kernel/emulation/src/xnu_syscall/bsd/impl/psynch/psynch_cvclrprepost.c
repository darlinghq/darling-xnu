#include <darling/emulation/xnu_syscall/bsd/impl/psynch/psynch_cvclrprepost.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/simple.h>

#include <darlingserver/rpc.h>

long sys_psynch_cvclrprepost(void* cv, uint32_t cvgen, uint32_t cvugen, uint32_t cvsgen, uint32_t prepocnt, uint32_t preposeq, uint32_t flags)
{
	uint32_t retval;
	int ret = dserver_rpc_psynch_cvclrprepost(cv, cvgen, cvugen, cvsgen, prepocnt, preposeq, flags, &retval);

	if (ret < 0) {
		__simple_printf("psynch_cvclrprepost failed internally: %d", ret);
		__simple_abort();
	}

	return (ret) ? ret : retval;
}
