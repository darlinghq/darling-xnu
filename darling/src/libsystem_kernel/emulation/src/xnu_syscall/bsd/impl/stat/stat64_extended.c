#include <darling/emulation/xnu_syscall/bsd/impl/stat/stat64_extended.h>

#include <darling/emulation/xnu_syscall/bsd/impl/stat/stat.h>
#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_stat64_extended(const char* path, struct stat64* stat, void* xsec, unsigned long* xsec_size)
{
	if (xsec_size)
		*xsec_size = 0;
	return sys_stat64(path, stat);
}
