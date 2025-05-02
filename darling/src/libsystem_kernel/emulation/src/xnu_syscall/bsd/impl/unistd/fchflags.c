#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchflags.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_fchflags(int fd, int flags)
{
	// TODO: We could implement this via ioctl(fd, FS_IOC_GETFLAGS/FS_IOC_SETFLAGS),
	// but it requires root and this functionality is not all that important.
	// 
	// Returning ENOTSUP indicates that this filesystem doesn't support file flags,
	// which is true on NFS for example, so applications must accept this error return.
	return -ENOTSUP;
}
