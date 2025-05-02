#include <darling/emulation/xnu_syscall/bsd/impl/misc/getlogin.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_shared_region_check_np(void* addr)
{
	return -EINVAL; // means: no shared region
}
