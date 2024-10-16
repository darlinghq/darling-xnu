#ifndef DARLING_MACH_EMULATION_MACH_TRAPS_H
#define DARLING_MACH_EMULATION_MACH_TRAPS_H

#include <mach/mach_traps.h>
#include <mach/vm_statistics.h>
#include <mach/kern_return.h>

typedef struct voucher_mach_msg_state_s *voucher_mach_msg_state_t;

kern_return_t clock_sleep_trap_impl(mach_port_name_t clock_name, sleep_type_t sleep_type, int sleep_sec, int sleep_nsec, mach_timespec_t	*wakeup_time);
mach_port_name_t host_self_trap_impl(void);
mach_msg_return_t mach_msg_overwrite_trap_impl(mach_msg_header_t *msg, mach_msg_option_t option, mach_msg_size_t send_size, mach_msg_size_t rcv_size, mach_port_name_t rcv_name, mach_msg_timeout_t timeout, mach_port_name_t notify, mach_msg_header_t *rcv_msg, mach_msg_size_t rcv_limit);
mach_msg_return_t mach_msg_trap_impl(mach_msg_header_t *msg, mach_msg_option_t option, mach_msg_size_t send_size, mach_msg_size_t rcv_size, mach_port_name_t rcv_name, mach_msg_timeout_t timeout, mach_port_name_t notify);
mach_port_name_t mach_reply_port_impl(void);
boolean_t swtch_impl(void);
boolean_t swtch_pri_impl(int pri);
kern_return_t syscall_thread_switch_impl(mach_port_name_t thread_name, int option, mach_msg_timeout_t option_time);
mach_port_name_t task_self_trap_impl(void);
kern_return_t thread_get_special_reply_port_impl(void);
mach_port_name_t thread_self_trap_impl(void);

// Obsolete interfaces.

kern_return_t bsdthread_terminate_trap_impl(uintptr_t stackaddr, unsigned long freesize, mach_port_name_t thread, mach_port_name_t sem);
kern_return_t mach_generate_activity_id_impl(mach_port_name_t task, int i, uint64_t* id);
kern_return_t pid_for_task_impl(mach_port_name_t t, int *x);
kern_return_t task_for_pid_impl(mach_port_name_t target_tport, int pid, mach_port_name_t *t);
kern_return_t task_name_for_pid_impl(mach_port_name_t target_tport, int pid, mach_port_name_t *tn);

// Non-syscall methods

voucher_mach_msg_state_t voucher_mach_msg_adopt(mach_msg_header_t *msg);
void voucher_mach_msg_revert(voucher_mach_msg_state_t state);
boolean_t voucher_mach_msg_set(mach_msg_header_t *msg);

#endif // DARLING_MACH_EMULATION_MACH_TRAPS_H
