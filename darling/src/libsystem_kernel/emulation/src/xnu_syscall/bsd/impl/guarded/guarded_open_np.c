#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_open_np.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>

long sys_guarded_open_np(const char* filename, const uint64_t* guardid, unsigned int guardflags, int flags, unsigned int mode)
{
	// XNU copies the guard in and fails with EFAULT when it cannot be read
	if (!guardid)
		return -EFAULT;

	return sys_open(filename, flags, mode);
}
