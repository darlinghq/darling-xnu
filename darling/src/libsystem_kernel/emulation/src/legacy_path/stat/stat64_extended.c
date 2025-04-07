#include <darling/emulation/legacy_path/stat/stat64_extended.h>

#include <darling/emulation/legacy_path/stat/stat.h>
#include <darling/emulation/legacy_path/stat/common.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_stat64_extended(const char* path, struct stat64* stat, void* xsec, unsigned long* xsec_size)
{
	if (xsec_size)
		*xsec_size = 0;
	return sys_stat64(path, stat);
}


