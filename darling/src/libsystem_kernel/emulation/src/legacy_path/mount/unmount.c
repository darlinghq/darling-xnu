#include <darling/emulation/legacy_path/mount/unmount.h>

#include <sys/errno.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_unmount(const char* path, int flags)
{
	return -EBUSY;
}

