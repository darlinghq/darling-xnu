#ifndef _DARLING_EMULATION_COMMON_GUARDED_GUARD_ARGS_H_
#define _DARLING_EMULATION_COMMON_GUARDED_GUARD_ARGS_H_

#include <stdint.h>

// the guardflags XNU accepts in guarded_open_np and guarded_kqueue_np (bsd/sys/guarded.h)
#define DARLING_GUARDFLAG_CLOSE      (1u << 0)
#define DARLING_GUARDFLAG_DUP        (1u << 1)
#define DARLING_GUARDFLAG_SOCKET_IPC (1u << 2)
#define DARLING_GUARDFLAG_FILEPORT   (1u << 3)
#define DARLING_GUARDFLAG_WRITE      (1u << 4)
#define DARLING_GUARDFLAG_ALL        (DARLING_GUARDFLAG_CLOSE | DARLING_GUARDFLAG_DUP | \
	DARLING_GUARDFLAG_SOCKET_IPC | DARLING_GUARDFLAG_FILEPORT | DARLING_GUARDFLAG_WRITE)

// Returns 0 or a negative BSD errno, in the order XNU checks: guardflags must include GUARD_DUP and
// no unknown bit (EINVAL), the guard must be readable (EFAULT) and must not be zero (EINVAL).
// The guard is only read. Readability is checked for NULL only: an unmapped non-NULL guard
// faults here, because there is no copyin primitive that works under a seccomp profile that
// blocks process_vm_readv.
static inline long guard_args_validate(const uint64_t* guardid, unsigned int guardflags)
{
	if ((guardflags & DARLING_GUARDFLAG_DUP) == 0 || (guardflags & ~DARLING_GUARDFLAG_ALL) != 0)
		return -22; // EINVAL
	if (!guardid)
		return -14; // EFAULT
	if (*(const volatile uint64_t*) guardid == 0)
		return -22; // EINVAL
	return 0;
}

#endif
