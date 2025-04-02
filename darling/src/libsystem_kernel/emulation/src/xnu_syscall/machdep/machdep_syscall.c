#include <emulation/xnu_syscall/machdep/machdep_syscall.h>

#include <emulation/common/simple.h>
#include <emulation/conversion/methods/errno.h>

long __darling_machdep_unknown_sysnum(int nr, ...)
{
	__simple_printf("Unimplemented machdep trap (%d)\n", nr);
	return -BSD_ENOSYS;
}