#include <darling/emulation/linux_premigration/ext/mremap.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern long cerror(int __err);

VISIBLE
long __linux_mremap(void* old_addr, unsigned long old_size, unsigned long new_size, int flags, void* new_address)
{
	long rv;

	rv = LINUX_SYSCALL(__NR_mremap, old_addr, old_size, new_size, flags, new_address);
	if (rv < 0 && rv >= -4095)
	{
		cerror(errno_linux_to_bsd(-rv));
		return -1;
	}

	return rv;
}
