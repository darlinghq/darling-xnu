#include <darling/emulation/xnu_syscall/bsd/impl/misc/setlogin.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_setlogin(const char* buf)
{
	return 0;
}
