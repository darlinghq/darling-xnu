#include <darling/emulation/legacy_path/ext/sysinfo.h>

#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

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

