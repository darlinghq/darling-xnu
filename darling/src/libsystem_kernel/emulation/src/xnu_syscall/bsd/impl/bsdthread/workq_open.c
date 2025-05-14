#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/workq_open.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_workq_open(void)
{
	return 0;
}
