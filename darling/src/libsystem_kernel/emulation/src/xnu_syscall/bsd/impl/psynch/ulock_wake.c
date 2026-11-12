#include <darling/emulation/xnu_syscall/bsd/impl/psynch/ulock_wake.h>

#include <sys/errno.h>
#include <stdbool.h>
#include <limits.h>

#include <darling/emulation/xnu_syscall/bsd/impl/psynch/ulock_wait.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_ulock_wake(uint32_t operation, void* addr, uint64_t wake_value)
{
	int ret, op;

	op = operation & XNU_UL_OPCODE_MASK;
	if (op == XNU_UL_COMPARE_AND_WAIT || op == XNU_UL_UNFAIR_LOCK ||
		op == XNU_UL_COMPARE_AND_WAIT_SHARED || op == XNU_UL_UNFAIR_LOCK64_SHARED ||
		op == XNU_UL_COMPARE_AND_WAIT64 || op == XNU_UL_COMPARE_AND_WAIT64_SHARED)
	{
		int value;

		if (operation & XNU_ULF_WAKE_ALL)
			value = INT_MAX;
		else if (operation & XNU_ULF_WAKE_THREAD)
			value = INT_MAX; // incorrect
		else
			value = 1;

		// A 64-bit wait is queued on both halves of the word (see ulock_wait64()), so waking the
		// address reaches it. The shared operations wait on the process-shared futex key.
		ret = LINUX_SYSCALL(__NR_futex, addr,
			FUTEX_WAKE | (xnu_ulock_op_is_shared(op) ? 0 : FUTEX_PRIVATE_FLAG),
			value);

		// ulock_wake reports ENOENT when no thread was waiting
		if (ret == 0)
			return -ENOENT;
	}
	else
		return -EINVAL;

	// Returned verbatim to ULF_NO_ERRNO callers, which switch on the exact -errno.
	if (ret < 0)
	{
		ret = errno_linux_to_bsd(ret);
	} else {
		// callers of ulock_wake expect it to return 0 on success
		ret = 0;
	}

	return ret;
}
