#include <emulation/xnu_syscall/mach/mach_syscall.h>

#include <mach/kern_return.h>

#include <emulation/common/simple.h>

long __darling_mach_unknown_sysnum(int nr, ...)
{
	__simple_printf("Unimplemented mach trap (%d)\n", nr);
	return KERN_FAILURE;
}