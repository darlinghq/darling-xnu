#include <darling/emulation/xnu_syscall/bsd/impl/guarded/change_fdguard_np.h>

#include <sys/errno.h>
#include <sys/fcntl.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>

// Darling does not keep per-descriptor guard state for callers, so the guard
// values are never compared and the validation errors of XNU (EINVAL, EPERM)
// are not produced. The mode is read from which guards were passed, as in the
// examples in bsd/kern/kern_guarded.c: a new guard on an unguarded fd, or no
// new guard to remove one. Only the fd flag side effects are modelled.
long sys_change_fdguard_np(int fd, const uint64_t* guard, unsigned int guardflags,
	const uint64_t* nguard, unsigned int nguardflags, int* fdflagsp)
{
	// the flags the caller wants restored when the guard is removed
	int new_flags = fdflagsp ? *fdflagsp : 0;
	long old = sys_fcntl(fd, F_GETFD, 0);

	if (old < 0)
		return old;

	if (fdflagsp)
		*fdflagsp = (old & FD_CLOEXEC) ? FD_CLOEXEC : 0;

	// guarding forces close-on-exec on; unguarding restores the saved state
	int cloexec = nguard ? FD_CLOEXEC : (new_flags & FD_CLOEXEC);
	long rv = sys_fcntl(fd, F_SETFD, cloexec);

	return rv < 0 ? rv : 0;
}
