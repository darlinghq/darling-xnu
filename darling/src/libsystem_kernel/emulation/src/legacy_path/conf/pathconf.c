#include <darling/emulation/legacy_path/conf/pathconf.h>

#include <darling/emulation/legacy_path/conf/fpathconf.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_pathconf(const char* file, int name)
{
	return sys_fpathconf(-1, name);
}

