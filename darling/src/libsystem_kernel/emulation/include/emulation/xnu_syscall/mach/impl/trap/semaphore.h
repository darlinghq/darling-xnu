#ifndef DARLING_MACH_EMULATION_SEMAPHORE_H
#define DARLING_MACH_EMULATION_SEMAPHORE_H

#include <mach/kern_return.h>
#include <mach/mach_traps.h>

kern_return_t semaphore_signal_trap_impl(mach_port_name_t signal_name);
kern_return_t semaphore_signal_all_trap_impl(mach_port_name_t signal_name);
kern_return_t semaphore_signal_thread_trap_impl(mach_port_name_t signal_name, mach_port_name_t thread_name);
kern_return_t semaphore_wait_trap_impl(mach_port_name_t wait_name);
kern_return_t semaphore_wait_signal_trap_impl(mach_port_name_t wait_name, mach_port_name_t signal_name);
kern_return_t semaphore_timedwait_trap_impl(mach_port_name_t wait_name, unsigned int sec, clock_res_t nsec);
kern_return_t semaphore_timedwait_signal_trap_impl(mach_port_name_t wait_name, mach_port_name_t signal_name, unsigned int sec, clock_res_t nsec);

#endif // DARLING_MACH_EMULATION_SEMAPHORE_H
