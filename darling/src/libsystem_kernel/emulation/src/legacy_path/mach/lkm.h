#ifndef _LKM_H
#define _LKM_H

#include <os/tsd.h>

#define DARLING_MACH_API_BASE		0x1000
enum { NR_get_api_version = DARLING_MACH_API_BASE,
	NR_mach_reply_port,
	NR__kernelrpc_mach_port_mod_refs,
	NR_task_self_trap,
	NR_host_self_trap,
	NR__kernelrpc_mach_port_allocate,
	NR_mach_msg_overwrite_trap,
	NR__kernelrpc_mach_port_deallocate,
	NR__kernelrpc_mach_port_destroy,
	NR_semaphore_signal_trap,
	NR_semaphore_signal_all_trap,
	NR_semaphore_wait_trap,
	NR_semaphore_wait_signal_trap,
	NR_semaphore_timedwait_signal_trap,
	NR_semaphore_timedwait_trap,
	NR_bsd_ioctl_trap,
	NR_thread_self_trap,
	NR_bsdthread_terminate_trap,
	NR_psynch_mutexwait_trap, // 0x12
	NR_psynch_mutexdrop_trap,
	NR_pthread_kill_trap,
	NR_psynch_cvwait_trap, // 0x15
	NR_psynch_cvsignal_trap,
	NR_psynch_cvbroad_trap,
	NR_mk_timer_create_trap,
	NR_mk_timer_arm_trap,
	NR_mk_timer_cancel_trap,
	NR_mk_timer_destroy_trap,
	NR__kernelrpc_mach_port_move_member_trap,
	NR__kernelrpc_mach_port_insert_member_trap,
	NR__kernelrpc_mach_port_extract_member_trap,
	NR_thread_death_announce,
	NR__kernelrpc_mach_port_insert_right_trap, // 0x20
	NR_fork_wait_for_child,
	NR_task_for_pid_trap,
	NR_pid_for_task_trap,
	NR_set_dyld_info,
	NR_stop_after_exec,
	NR_kernel_printk, // 0x28
	NR_path_at,
	NR_psynch_rw_rdlock,
	NR_psynch_rw_wrlock,
	NR_psynch_rw_unlock,
	NR_psynch_cvclrprepost,
	NR_get_tracer,
	NR_tid_for_thread,
	NR_getuidgid,
	NR_setuidgid,
	NR_task_name_for_pid_trap,
	NR_set_tracer,
	NR_pthread_markcancel,
	NR_pthread_canceled,
	NR_pid_get_state,
	NR_started_suspended,
	NR_task_64bit,
	NR__kernelrpc_mach_vm_allocate_trap,
	NR__kernelrpc_mach_vm_deallocate_trap,
	NR_last_triggered_watchpoint,
	NR_vchroot, // fd as paramv
	NR_vchroot_expand,
	NR_vchroot_fdpath,
	NR_handle_to_path,
	NR_fileport_makeport,
	NR_fileport_makefd,
	NR_sigprocess,
	NR_ptrace_thupdate,
	NR_ptrace_sigexc,
	NR_thread_suspended,
	NR_set_thread_handles,
	NR_thread_get_special_reply_port,
	NR__kernelrpc_mach_port_request_notification_trap,
	NR__kernelrpc_mach_port_type_trap,
	NR__kernelrpc_mach_port_get_attributes_trap,
	NR__kernelrpc_mach_port_construct_trap,
	NR__kernelrpc_mach_port_destruct_trap,
	NR__kernelrpc_mach_port_guard_trap,
	NR__kernelrpc_mach_port_unguard_trap,
	NR_kqueue_create,
	NR_kevent,
	NR_kevent64,
	NR_kevent_qos,
	NR_closing_descriptor,
	DARLING_MACH_API_ALMOST_COUNT, // don't use this directly (it's offset by the API base number); use `DARLING_MACH_API_COUNT`
};
#define DARLING_MACH_API_COUNT (DARLING_MACH_API_ALMOST_COUNT - DARLING_MACH_API_BASE)

void mach_driver_init(const char** applep);
int lkm_call(int call_nr, void* arg);
int lkm_call_raw(int call_nr, void* arg); // w/o errno translation
int mach_driver_get_fd(void);

#endif

