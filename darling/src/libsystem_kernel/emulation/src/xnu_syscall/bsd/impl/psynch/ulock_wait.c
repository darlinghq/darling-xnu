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

// futex_waitv(2) always uses 64-bit time and pointer fields, also for 32-bit callers.
struct waitv_timespec
{
	int64_t tv_sec;
	int64_t tv_nsec;
};

struct futex_waitv_entry
{
	uint64_t val;
	uint64_t uaddr;
	uint32_t flags;
	uint32_t reserved;
};

#ifndef __NR_futex_waitv
#	define __NR_futex_waitv 449
#endif
#define WAITV_FUTEX_32 2
#define MONOTONIC_CLOCK 1

// A wait on a 64-bit word.
//
// A futex compares only 32 bits, so waiting on the low half alone would sleep through a change
// of the high half (and miss its wake-up if the change happens between a separate 64-bit check and
// the futex call). futex_waitv() queues the thread on both halves of the word and compares both
// under the futex hash bucket locks, so the 64-bit comparison and the decision to sleep are one
// atomic step. A wake on the address of the word (its low half) reaches the thread.
// The timeout of futex_waitv() is absolute.
static long ulock_wait64(void* addr, uint64_t value, uint64_t timeout_ns, uint32_t futex_flags, bool cancel_point)
{
	struct futex_waitv_entry words[2];
	struct waitv_timespec abs_timeout;
	struct waitv_timespec* timeout = NULL;

	words[0].val = (uint32_t) value;
	words[0].uaddr = (uint64_t)(uintptr_t) addr;
	words[0].flags = WAITV_FUTEX_32 | futex_flags;
	words[0].reserved = 0;
	words[1].val = (uint32_t) (value >> 32);
	words[1].uaddr = (uint64_t)(uintptr_t) addr + 4;
	words[1].flags = WAITV_FUTEX_32 | futex_flags;
	words[1].reserved = 0;

	if (timeout_ns != 0)
	{
		struct timespec now;
		int64_t ns;

		LINUX_SYSCALL(__NR_clock_gettime, MONOTONIC_CLOCK, &now);
		ns = (int64_t) now.tv_nsec + (int64_t) (timeout_ns % 1000000000ULL);
		abs_timeout.tv_sec = (int64_t) now.tv_sec + (int64_t) (timeout_ns / 1000000000ULL) + ns / 1000000000;
		abs_timeout.tv_nsec = ns % 1000000000;
		timeout = &abs_timeout;
	}

	if (cancel_point)
		return pthread_cancelable_syscall((long) words, 2, 0, (long) timeout, MONOTONIC_CLOCK, 0, __NR_futex_waitv);
	return LINUX_SYSCALL(__NR_futex_waitv, words, 2, 0, timeout, MONOTONIC_CLOCK);
}

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
		// operations on memory shared between processes must use the shared futex key
		uint32_t futex_flags = xnu_ulock_op_is_shared(op) ? 0 : FUTEX_PRIVATE_FLAG;
		bool cancel_point = (operation & XNU_ULF_WAIT_CANCEL_POINT) != 0;

		ret = -LINUX_ENOSYS;
		if (xnu_ulock_op_is_64bit(op))
			ret = ulock_wait64(addr, value, timeout_ns, futex_flags, cancel_point);

		if (ret == -LINUX_ENOSYS)
		{
			// (Also the fallback of 64-bit waits on hosts older than Linux 5.16, which lack
			// futex_waitv: they wait on the low half only, after checking the high half.)
			if (xnu_ulock_op_is_64bit(op) && *(volatile uint32_t*) ((char*) addr + 4) != (uint32_t) (value >> 32))
				return 1;

			// like xnu's ulock_wait, a cancellation point wait acts on a pending cancel first,
			// and is interrupted (with EINTR) when the thread is marked for cancellation while it waits
			if (cancel_point)
				ret = pthread_cancelable_syscall((long) addr, FUTEX_WAIT | futex_flags,
					(long) value, (long) timeout, 0, 0, __NR_futex);
			else
				ret = LINUX_SYSCALL(__NR_futex, addr, FUTEX_WAIT | futex_flags,
					value, timeout);
		}

		// unlike ulock_wait(), futex(FUTEX_WAIT) does not return how many
		// other threads are now (still) waiting for the lock.
		//
		// This hack makes userspace believe that there are other pending threads
		// and always take the slow path, which is the safe thing to do if
		// we are unsure.
		if (ret >= 0 || ret == -LINUX_EAGAIN)
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
