#include <darling/emulation/legacy_path/unistd/setgroups.h>

#include <darling/emulation/legacy_path/unistd/getgroups.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_setgroups(unsigned int size, int* gidset)
{
	return __setgroups(size, (const unsigned int*)gidset);
}

