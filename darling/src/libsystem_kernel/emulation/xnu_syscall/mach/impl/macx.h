#ifndef DARLING_MACH_EMULATION_MACX_H
#define DARLING_MACH_EMULATION_MACX_H

#include <mach/mach_traps.h>
#include <mach/kern_return.h>

kern_return_t macx_swapon_impl(uint64_t filename, int flags, int size, int priority);
kern_return_t macx_swapoff_impl(uint64_t filename, int flags);
kern_return_t macx_triggers_impl(int hi_water, int low_water, int flags, mach_port_t alert_port);
kern_return_t macx_backing_store_suspend_impl(boolean_t suspend);
kern_return_t macx_backing_store_recovery_impl(int pid);

#endif // DARLING_MACH_EMULATION_MACX_H
