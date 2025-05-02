#include <darling/emulation/xnu_syscall/bsd/impl/unistd/setgroups.h>

#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getgroups.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_setgroups(unsigned int size, int* gidset)
{
	return __setgroups(size, (const unsigned int*)gidset);
}
