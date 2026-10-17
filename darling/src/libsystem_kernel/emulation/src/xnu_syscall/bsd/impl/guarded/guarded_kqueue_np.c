#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_kqueue_np.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kqueue.h>

long sys_guarded_kqueue_np(const uint64_t* guardid, unsigned guardflags)
{
	// XNU copies the guard in and fails with EFAULT when it cannot be read
	if (!guardid)
		return -EFAULT;

	return sys_kqueue();
}
