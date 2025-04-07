#include <darling/emulation/legacy_path/ext/sys/utsname.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

extern void cerror(int e);

int __linux_uname(struct linux_utsname* p)
{
	int rv;

	rv = LINUX_SYSCALL(__NR_uname, p);
	if (rv < 0)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}

