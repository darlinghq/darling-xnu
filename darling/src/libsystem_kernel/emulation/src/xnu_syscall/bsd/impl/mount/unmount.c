#include <darling/emulation/xnu_syscall/bsd/impl/mount/unmount.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_unmount(const char* path, int flags)
{
	return -EBUSY;
}
