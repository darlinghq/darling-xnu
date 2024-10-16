#include <sys/errno.h>

#include <emulation/common/simple.h>

long __darling_machdep_unknown_sysnum(int nr, ...)
{
	__simple_printf("Unimplemented machdep trap (%d)\n", nr);
	return -ENOSYS;
}