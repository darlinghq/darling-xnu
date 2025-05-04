#include <darling/emulation/linux_premigration/ext/syslog.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern long cerror(int __err);

VISIBLE
int __linux_syslog (int type, char* bufp, int len)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_syslog, type, bufp, len);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}
