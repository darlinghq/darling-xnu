#include <darling/emulation/xnu_syscall/bsd/impl/conf/pathconf.h>

#include <darling/emulation/xnu_syscall/bsd/impl/conf/fpathconf.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_pathconf(const char* file, int name)
{
	return sys_fpathconf(-1, name);
}
