#include "bsd_syscall.h"

#include <sys/errno.h>

#include <emulation/common/simple.h>

long __darling_bsd_unknown_sysnum(int nr, ...)
{
	__simple_printf("Unimplemented bsd syscall (%d)\n", nr);
	return -ENOSYS;
}