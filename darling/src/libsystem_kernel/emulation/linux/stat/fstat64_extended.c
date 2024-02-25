#include "fstat64_extended.h"
#include "fstat.h"
#include "common.h"
#include "../base.h"
#include "../errno.h"
#include <linux-syscalls/linux.h>

#if defined(__i386__) || defined(__x86_64__)
long sys_fstat64_extended(int fd, struct stat64* stat, void* xsec, unsigned long* xsec_size)
#elif defined(__arm64__)
long sys_fstat64_extended(int fd, struct stat* stat, void* xsec, unsigned long* xsec_size)
#endif
{
	if (xsec_size)
		*xsec_size = 0;
	return sys_fstat64(fd, stat);
}


