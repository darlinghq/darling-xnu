#include <darling/emulation/linux_premigration/ext/sysinfo.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern long cerror(int __err);

int __linux_sysinfo(struct sysinfo* info)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_sysinfo, info);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}
