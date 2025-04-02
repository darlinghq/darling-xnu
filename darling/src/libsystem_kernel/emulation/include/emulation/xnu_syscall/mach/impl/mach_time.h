#ifndef DARLING_MACH_EMULATION_MACH_TIME_H
#define DARLING_MACH_EMULATION_MACH_TIME_H

#include <mach/mach_time.h>
#include <mach/kern_return.h>

kern_return_t mach_timebase_info_trap_impl(mach_timebase_info_t info);
kern_return_t mach_wait_until_impl(uint64_t deadline);

#endif // DARLING_MACH_EMULATION_MACH_TIME_H
