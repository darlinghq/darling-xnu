#include <darling/emulation/legacy_path/misc/getlogin.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_shared_region_check_np(void* addr)
{
	return -EINVAL; // means: no shared region
}

