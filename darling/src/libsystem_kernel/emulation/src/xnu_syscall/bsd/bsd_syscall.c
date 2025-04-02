#include <emulation/xnu_syscall/bsd/bsd_syscall.h>

#include <emulation/common/simple.h>
#include <emulation/conversion/methods/errno.h>

long __darling_bsd_unknown_sysnum(int nr, ...)
{
	__simple_printf("Unimplemented bsd syscall (%d)\n", nr);
	return -BSD_ENOSYS;
}