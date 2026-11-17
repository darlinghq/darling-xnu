#ifndef _BSDTHREAD_CANCELLABLE_H
#define _BSDTHREAD_CANCELLABLE_H

#include <stdbool.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>

#include <darling/emulation/xnu_syscall/bsd/impl/bsdthread/pthread_canceled.h>

#define CANCELATION_POINT() \
	if (pthread_testcancel_presyscall()) \
		return -EINTR;

// LINUX_SYSCALL() for the body that serves both a cancellation point (sys_read()) and its $NOCANCEL variant
// (sys_read_nocancel()): with `cancelable` true, marking the thread for cancellation interrupts the
// syscall while it blocks and the result is -LINUX_EINTR, which the libc wrapper turns into the cancel
static inline long linux_syscall_maybe_cancelable(bool cancelable, long a1, long a2, long a3, long a4, long a5, long a6, int nr)
{
	if (cancelable)
		return pthread_cancelable_syscall_restart(a1, a2, a3, a4, a5, a6, nr);
	return linux_syscall(a1, a2, a3, a4, a5, a6, nr);
}

#define LINUX_SYSCALL_C0(c, nr) linux_syscall_maybe_cancelable(c, 0, 0, 0, 0, 0, 0, nr)
#define LINUX_SYSCALL_C1(c, nr, a1) linux_syscall_maybe_cancelable(c, (long)a1, 0, 0, 0, 0, 0, nr)
#define LINUX_SYSCALL_C2(c, nr, a1, a2) linux_syscall_maybe_cancelable(c, (long)a1, (long)a2, 0, 0, 0, 0, nr)
#define LINUX_SYSCALL_C3(c, nr, a1, a2, a3) linux_syscall_maybe_cancelable(c, (long)a1, (long)a2, (long)a3, 0, 0, 0, nr)
#define LINUX_SYSCALL_C4(c, nr, a1, a2, a3, a4) linux_syscall_maybe_cancelable(c, (long)a1, (long)a2, (long)a3, (long)a4, 0, 0, nr)
#define LINUX_SYSCALL_C5(c, nr, a1, a2, a3, a4, a5) linux_syscall_maybe_cancelable(c, (long)a1, (long)a2, (long)a3, (long)a4, (long)a5, 0, nr)
#define LINUX_SYSCALL_C6(c, nr, a1, a2, a3, a4, a5, a6) linux_syscall_maybe_cancelable(c, (long)a1, (long)a2, (long)a3, (long)a4, (long)a5, (long)a6, nr)
#define LINUX_SYSCALL_C(c, ...) __SYSCALL_CONCAT(LINUX_SYSCALL_C, __SYSCALL_NARGS(__VA_ARGS__))(c, __VA_ARGS__)

#endif // _BSDTHREAD_CANCELLABLE_H
