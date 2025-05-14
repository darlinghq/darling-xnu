#include <darling/emulation/xnu_syscall/bsd/impl/misc/iopolicysys.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/errno.h>

long sys_iopolicysys(int cmd, void* arg)
{
	return 0;
}
