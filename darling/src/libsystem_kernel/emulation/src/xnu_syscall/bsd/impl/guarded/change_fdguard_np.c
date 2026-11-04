#include <darling/emulation/xnu_syscall/bsd/impl/guarded/change_fdguard_np.h>

#include <stddef.h>

#include <sys/errno.h>
#include <sys/fcntl.h>

#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>

// bsd/sys/guarded.h only exposes these to PRIVATE builds; values and masks
// as in it and bsd/kern/kern_guarded.c
#define GUARD_CLOSE (1u << 0)
#define GUARD_DUP (1u << 1)
#define GUARD_SOCKET_IPC (1u << 2)
#define GUARD_FILEPORT (1u << 3)
#define GUARD_WRITE (1u << 4)
#define GUARD_REQUIRED GUARD_DUP
#define GUARD_ALL (GUARD_REQUIRED | GUARD_CLOSE | GUARD_SOCKET_IPC | GUARD_FILEPORT | GUARD_WRITE)

// Darling does not keep per-descriptor guard state for callers, so the guard
// values are never compared and the EPERM of XNU is not produced. The EINVAL
// checks that need no guard state are made before the fd is touched, in the
// order of bsd/kern/kern_guarded.c. The mode is read from which guards were
// passed, as in the examples there: a new guard on an unguarded fd, or no
// new guard to remove one. Only the fd flag side effects are modelled.
long sys_change_fdguard_np(int fd, const uint64_t* guard, unsigned int guardflags,
	const uint64_t* nguard, unsigned int nguardflags, int* fdflagsp)
{
	// the flags the caller wants restored when the guard is removed
	int new_flags = fdflagsp ? *fdflagsp : 0;
	// both calls use the nocancel variant so no cancellation point lies between them
	long old = sys_fcntl_nocancel(fd, F_GETFD, 0);

	if (old < 0)
		return old;

	// a guard and its flags go together, and the fd must be changed in some way
	if ((guard == NULL) != (guardflags == 0))
		return -EINVAL;
	if (guard == NULL && nguard == NULL)
		return -EINVAL;

	if (nguard != NULL) {
		if (*nguard == 0)
			return -EINVAL;
		if ((nguardflags & GUARD_REQUIRED) != GUARD_REQUIRED || (nguardflags & ~GUARD_ALL) != 0)
			return -EINVAL;
	} else if (nguardflags != 0) {
		return -EINVAL;
	}

	if (fdflagsp)
		*fdflagsp = (old & FD_CLOEXEC) ? FD_CLOEXEC : 0;

	// guarding forces close-on-exec on; unguarding restores the saved state
	int cloexec = nguard ? FD_CLOEXEC : (new_flags & FD_CLOEXEC);
	long rv = sys_fcntl_nocancel(fd, F_SETFD, cloexec);

	return rv < 0 ? rv : 0;
}
