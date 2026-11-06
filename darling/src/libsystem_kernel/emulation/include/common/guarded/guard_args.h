#ifndef _DARLING_EMULATION_COMMON_GUARDED_GUARD_ARGS_H_
#define _DARLING_EMULATION_COMMON_GUARDED_GUARD_ARGS_H_

#include <stdint.h>
#include <sys/errno.h>

// the guardflags XNU accepts in guarded_open_np and guarded_kqueue_np (bsd/sys/guarded.h)
#define DARLING_GUARDFLAG_CLOSE      (1u << 0)
#define DARLING_GUARDFLAG_DUP        (1u << 1)
#define DARLING_GUARDFLAG_SOCKET_IPC (1u << 2)
#define DARLING_GUARDFLAG_FILEPORT   (1u << 3)
#define DARLING_GUARDFLAG_WRITE      (1u << 4)
#define DARLING_GUARDFLAG_ALL        (DARLING_GUARDFLAG_CLOSE | DARLING_GUARDFLAG_DUP | \
	DARLING_GUARDFLAG_SOCKET_IPC | DARLING_GUARDFLAG_FILEPORT | DARLING_GUARDFLAG_WRITE)

// FUTEX_WAIT | FUTEX_PRIVATE_FLAG
#define DARLING_FUTEX_WAIT_PRIVATE 128

// Returns 0 or a negative BSD errno, in the order XNU checks: guardflags must include GUARD_DUP and
// no unknown bit (EINVAL), the guard must be readable (EFAULT) and must not be zero (EINVAL).
// The guard is only read. Readability stands in for XNU's copyin: a futex wait with a zero timeout
// reads the word without side effects and fails with EFAULT on an unmapped or PROT_NONE page
// (any other result means the word could be read). futex is allowed by container seccomp profiles,
// unlike process_vm_readv. futex needs a 4-byte aligned word, so each half of the guard is probed;
// a misaligned guard skips the probe and faults on the read below. Callers include
// <darling/emulation/common/base.h> and the linux syscall numbers first.
static inline long guard_args_validate(const uint64_t* guardid, unsigned int guardflags)
{
	if ((guardflags & DARLING_GUARDFLAG_DUP) == 0 || (guardflags & ~DARLING_GUARDFLAG_ALL) != 0)
		return -EINVAL;
	if (!guardid)
		return -EFAULT;
	if (((uintptr_t) guardid & 3) == 0)
	{
		const uint32_t* halves = (const uint32_t*) guardid;
		for (int i = 0; i < 2; ++i)
		{
			long timeout[2] = { 0, 0 };
			if (LINUX_SYSCALL(__NR_futex, halves + i, DARLING_FUTEX_WAIT_PRIVATE, 0, timeout) == -EFAULT)
				return -EFAULT;
		}
	}
	if (*(const volatile uint64_t*) guardid == 0)
		return -EINVAL;
	return 0;
}

#endif
