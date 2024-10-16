#include "macx.h"

#include <emulation/xnu_syscall/mach/mach_syscall.h>

kern_return_t macx_swapon_impl(uint64_t filename, int flags, int size, int priority)
{
	UNIMPLEMENTED_TRAP();
	return KERN_FAILURE;
}

kern_return_t macx_swapoff_impl(uint64_t filename, int flags)
{
	UNIMPLEMENTED_TRAP();
	return KERN_FAILURE;
}

kern_return_t macx_triggers_impl(int hi_water, int low_water, int flags, mach_port_t alert_port)
{
	UNIMPLEMENTED_TRAP();
	return KERN_FAILURE;
}

kern_return_t macx_backing_store_suspend_impl(boolean_t suspend)
{
	UNIMPLEMENTED_TRAP();
	return KERN_FAILURE;
}

kern_return_t macx_backing_store_recovery_impl(int pid)
{
	UNIMPLEMENTED_TRAP();
	return KERN_FAILURE;
}