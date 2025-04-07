#include <darling/emulation/legacy_path/unistd/fchflags.h>

#include <sys/errno.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_fchflags(int fd, int flags)
{
	// TODO: We could implement this via ioctl(fd, FS_IOC_GETFLAGS/FS_IOC_SETFLAGS),
	// but it requires root and this functionality is not all that important.
	// 
	// Returning ENOTSUP indicates that this filesystem doesn't support file flags,
	// which is true on NFS for example, so applications must accept this error return.
	return -ENOTSUP;
}

