#include <emulation/xnu_syscall/mach/impl/mach_other.h>

// TODO: To make it eaiser to port this code into the Linux side (when we implement
// syscall user dispatch support in mldr), we should avoid directly using macOS definitions.
// In other words, `BSD_MAP_ANON` instead of `MAP_ANON`

#include <fcntl.h>
#include <unistd.h>
#include <mach/mach_traps.h>

#include <darlingserver/rpc.h>

#include <emulation/common/base.h>
#include <emulation/common/simple.h>
#include <emulation/conversion/methods/errno.h>
#include <emulation/linux_api/c_api/nanosleep.h>
#include <emulation/linux_api/c_api/sched_yield.h>
#include <emulation/xnu_syscall/mach/mach_syscall.h>


kern_return_t clock_sleep_trap_impl(
	mach_port_name_t clock_name, sleep_type_t sleep_type, int sleep_sec,
	int sleep_nsec, mach_timespec_t	*wakeup_time
) {
	UNIMPLEMENTED_TRAP();
	return KERN_FAILURE;
}

mach_port_name_t host_self_trap_impl(void)
{
	unsigned int port_name;
	if (dserver_rpc_host_self_trap(&port_name) != 0) {
		port_name = MACH_PORT_NULL;
	}
	return port_name;
}

mach_msg_return_t mach_msg_overwrite_trap_impl(
	mach_msg_header_t *msg, mach_msg_option_t option, mach_msg_size_t send_size,
	mach_msg_size_t rcv_size, mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
	mach_port_name_t notify, mach_msg_header_t *rcv_msg, mach_msg_size_t rcv_limit
) {
	int code;

retry:
	code = dserver_rpc_mach_msg_overwrite(msg, option, send_size, rcv_size, rcv_name, timeout, notify, rcv_msg);

	if (code < 0) {
		if (code == -LINUX_EINTR) {
			// when the RPC call returns EINTR, it means we didn't manage to send the RPC message to the server;
			// when the RPC receive operation receives EINTR, it retries the call, meaning we should never see EINTR from an RPC receive.
			// therefore, if we wanted to both send and receive a message, this means the send (which is performed first) was interrupted.
			//
			// we also need to check if the caller wants to know about interrupts. if they want send interrupts, we tell them.
			// if they want receive interrupts, we tell them. otherwise, we retry the call.
			if ((option & MACH_SEND_MSG) != 0 && (option & MACH_SEND_INTERRUPT) != 0) {
				return MACH_SEND_INTERRUPTED;
			} else if ((option & MACH_RCV_MSG) != 0 && (option & MACH_RCV_INTERRUPT) != 0) {
				return MACH_RCV_INTERRUPTED;
			} else {
				goto retry;
			}
		}
		__simple_printf("mach_msg_overwrite failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

mach_msg_return_t mach_msg_trap_impl(
	mach_msg_header_t *msg, mach_msg_option_t option, mach_msg_size_t send_size,
	mach_msg_size_t rcv_size, mach_port_name_t rcv_name, mach_msg_timeout_t timeout,
	mach_port_name_t notify
) {
	return mach_msg_overwrite_trap_impl(
		msg, option, send_size, rcv_size,
		rcv_name, timeout, notify, msg, 0
	);
}

mach_port_name_t mach_reply_port_impl(void)
{
	unsigned int port_name;
	if (dserver_rpc_mach_reply_port(&port_name) != 0) {
		port_name = MACH_PORT_NULL;
	}
	return port_name;
}

boolean_t swtch_impl(void)
{
	__linux_sched_yield();
	return 0;
}

boolean_t swtch_pri_impl(int pri)
{
	__linux_sched_yield();
	return 0;
}

kern_return_t syscall_thread_switch_impl(mach_port_name_t thread_name, int option, mach_msg_timeout_t option_time)
{
	struct timespec tv = {
		.tv_sec = 0,
		.tv_nsec = 1000000
	};
	// Sleep for 1ms
	__linux_nanosleep(&tv, &tv);

	// TODO: we could implement this with yield_to() in LKM
	return KERN_SUCCESS;
}

mach_port_name_t task_self_trap_impl(void)
{
	unsigned int port_name;
	if (dserver_rpc_task_self_trap(&port_name) != 0) {
		port_name = MACH_PORT_NULL;
	}
	return port_name;
}

kern_return_t thread_get_special_reply_port_impl(void)
{
	unsigned int port_name;
	if (dserver_rpc_thread_get_special_reply_port(&port_name) != 0) {
		port_name = MACH_PORT_NULL;
	}
	return port_name;
}

mach_port_name_t thread_self_trap_impl(void)
{
	unsigned int port_name;
	if (dserver_rpc_thread_self_trap(&port_name) != 0) {
		port_name = MACH_PORT_NULL;
	}
	return port_name;
}


/*
 *	Obsolete interfaces.
 */


kern_return_t bsdthread_terminate_trap_impl(
	uintptr_t stackaddr, unsigned long freesize, mach_port_name_t thread,
	mach_port_name_t sem
) {
	UNIMPLEMENTED_TRAP();
	return KERN_FAILURE;
}

kern_return_t mach_generate_activity_id_impl(mach_port_name_t task, int i, uint64_t* id)
{
	UNIMPLEMENTED_TRAP();
	return KERN_FAILURE;
}

kern_return_t pid_for_task_impl(mach_port_name_t t, int *x)
{
	int code = dserver_rpc_pid_for_task(t, x);

	if (code < 0) {
		__simple_printf("pid_for_task failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t task_for_pid_impl(mach_port_name_t target_tport, int pid, mach_port_name_t *t)
{
	int code = dserver_rpc_task_for_pid(target_tport, pid, t);

	if (code < 0) {
		__simple_printf("task_for_pid failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t task_name_for_pid_impl(mach_port_name_t target_tport, int pid, mach_port_name_t *tn)
{
	int code = dserver_rpc_task_name_for_pid(target_tport, pid, tn);

	if (code < 0) {
		__simple_printf("task_name_for_pid failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}


/*
 * Non-syscall methods
 */

// I'm not sure which place would be better for these methods

VISIBLE
voucher_mach_msg_state_t voucher_mach_msg_adopt(mach_msg_header_t *msg)
{
	// UNIMPLEMENTED_TRAP();
	return NULL;
}

VISIBLE
void voucher_mach_msg_revert(voucher_mach_msg_state_t state)
{
	// UNIMPLEMENTED_TRAP();
}

VISIBLE
boolean_t voucher_mach_msg_set(mach_msg_header_t *msg)
{
	// UNIMPLEMENTED_TRAP();
	return 0;
}