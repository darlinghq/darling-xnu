#include <darling/emulation/xnu_syscall/bsd/impl/unistd/initgroups.h>

#include <darling/emulation/xnu_syscall/bsd/impl/unistd/setgroups.h>
#include <darling/emulation/common/base.h>

long sys_initgroups(unsigned int size, int* gidset, int gmuid)
{
	return sys_setgroups(size, gidset);
}
