#include <darling/emulation/xnu_syscall/bsd/impl/psynch/ulock_wait.h>

#include <sys/errno.h>
#include <stdbool.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>

struct timespec
{
	long tv_sec;
	long tv_nsec;
};

// timeout_ns is in ns
long sys_ulock_wait2(uint32_t operation, void* addr, uint64_t value, uint64_t timeout_ns, uint64_t value2)
{
	int ret, op;
	struct timespec ts;

	(void)value2;

	if (timeout_ns > 0)
	{
		ts.tv_sec = timeout_ns / 1000000000ULL;
		ts.tv_nsec = timeout_ns % 1000000000ULL;
	}

	op = operation & XNU_UL_OPCODE_MASK;
	if (op == XNU_UL_COMPARE_AND_WAIT || op == XNU_UL_UNFAIR_LOCK ||
		op == XNU_UL_COMPARE_AND_WAIT_SHARED || op == XNU_UL_UNFAIR_LOCK64_SHARED ||
		op == XNU_UL_COMPARE_AND_WAIT64 || op == XNU_UL_COMPARE_AND_WAIT64_SHARED)
	{
		struct timespec* timeout = (timeout_ns != 0) ? &ts : NULL;

		// like xnu's ulock_wait, a cancellation point wait acts on a pending cancel first,
		// and is interrupted (with EINTR) when the thread is marked for cancellation while it waits
		if (operation & XNU_ULF_WAIT_CANCEL_POINT)
			ret = pthread_cancelable_syscall((long) addr, FUTEX_WAIT | FUTEX_PRIVATE_FLAG,
				(long) value, (long) timeout, 0, 0, __NR_futex);
		else
			ret = LINUX_SYSCALL(__NR_futex, addr, FUTEX_WAIT | FUTEX_PRIVATE_FLAG,
				value, timeout);

		// unlike ulock_wait(), futex(FUTEX_WAIT) does not return how many
		// other threads are now (still) waiting for the lock.
		//
		// This hack makes userspace believe that there are other pending threads
		// and always take the slow path, which is the safe thing to do if
		// we are unsure.
		if (ret == 0 || ret == -LINUX_EAGAIN)
			ret = 1;
	}
	else
		return -EINVAL;

	// Returned verbatim to ULF_NO_ERRNO callers, which switch on the exact -errno.
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

// timeout is in us
long sys_ulock_wait(uint32_t operation, void* addr, uint64_t value, uint32_t timeout)
{
	uint64_t timeout_ns = (uint64_t)timeout * 1000ULL;
	return sys_ulock_wait2(operation, addr, value, timeout_ns, 0);
}
