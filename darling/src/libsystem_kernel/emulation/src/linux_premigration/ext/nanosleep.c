#include <darling/emulation/linux_premigration/ext/sys/utsname.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern void cerror(int e);

int __linux_nanosleep(struct timespec* ts, struct timespec* rem)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_nanosleep, ts, rem);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}
