#include <darling/emulation/legacy_path/misc/setlogin.h>

#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_setlogin(const char* buf)
{
	return 0;
}

