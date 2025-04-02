#ifndef DARLING_MACH_EMULATION_MK_TIMER_H
#define DARLING_MACH_EMULATION_MK_TIMER_H

#include <mach/mach_traps.h>

mach_port_name_t mk_timer_create_impl(void);
kern_return_t mk_timer_destroy_impl(mach_port_name_t name);
kern_return_t mk_timer_arm_impl(mach_port_name_t name, uint64_t expire_time);
kern_return_t mk_timer_cancel_impl(mach_port_name_t name, uint64_t *result_time);

#endif // DARLING_MACH_EMULATION_MK_TIMER_H
