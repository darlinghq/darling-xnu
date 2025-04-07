#include <darling/emulation/legacy_path/stat/fstat64_extended.h>

#include <darling/emulation/legacy_path/stat/fstat.h>
#include <darling/emulation/legacy_path/stat/common.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_fstat64_extended(int fd, struct stat64* stat, void* xsec, unsigned long* xsec_size)
{
	if (xsec_size)
		*xsec_size = 0;
	return sys_fstat64(fd, stat);
}


