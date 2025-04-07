#include <darling/emulation/legacy_path/unistd/initgroups.h>

#include <darling/emulation/legacy_path/unistd/setgroups.h>
#include <darling/emulation/legacy_path/base.h>

long sys_initgroups(unsigned int size, int* gidset, int gmuid)
{
	return sys_setgroups(size, gidset);
}

