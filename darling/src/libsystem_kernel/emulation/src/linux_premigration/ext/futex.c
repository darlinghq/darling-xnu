#include <darling/emulation/linux_premigration/ext/futex.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern long cerror(int __err);

VISIBLE
int __linux_futex (int *uaddr, int op, int val, const struct timespec *timeout,
		                 int *uaddr2, int val3)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_futex, uaddr, op, val, timeout, uaddr2, val3);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}

VISIBLE
int __linux_futex_reterr(int *uaddr, int op, int val, const struct timespec *timeout, int *uaddr2, int val3) {
	int rv = LINUX_SYSCALL(__NR_futex, uaddr, op, val, timeout, uaddr2, val3);
	return (rv < 0) ? errno_linux_to_bsd(rv) : rv;
};
