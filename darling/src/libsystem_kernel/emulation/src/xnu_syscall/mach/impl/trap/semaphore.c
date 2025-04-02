#include <emulation/xnu_syscall/mach/impl/trap/semaphore.h>

#include <darlingserver/rpc.h>

#include <emulation/common/simple.h>
#include <emulation/conversion/methods/errno.h>
#include <emulation/xnu_syscall/mach/mach_syscall.h>

kern_return_t semaphore_signal_trap_impl(mach_port_name_t signal_name)
{
	int code = dserver_rpc_semaphore_signal(signal_name);

	if (code < 0) {
		__simple_printf("semaphore_signal failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}
					      
kern_return_t semaphore_signal_all_trap_impl(mach_port_name_t signal_name)
{
	int code = dserver_rpc_semaphore_signal_all(signal_name);

	if (code < 0) {
		__simple_printf("semaphore_signal_all failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t semaphore_signal_thread_trap_impl(
	mach_port_name_t signal_name, mach_port_name_t thread_name
) {
	UNIMPLEMENTED_TRAP();
	return KERN_FAILURE;
}

kern_return_t semaphore_wait_trap_impl(mach_port_name_t wait_name)
{
	int code = dserver_rpc_semaphore_wait(wait_name);

	if (code < 0) {
		if (code == -LINUX_EINTR) {
			return KERN_ABORTED;
		}
		__simple_printf("semaphore_wait failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t semaphore_wait_signal_trap_impl(
	mach_port_name_t wait_name, mach_port_name_t signal_name
) {
	int code = dserver_rpc_semaphore_wait_signal(wait_name, signal_name);

	if (code < 0) {
		if (code == -LINUX_EINTR) {
			return KERN_ABORTED;
		}
		__simple_printf("semaphore_wait_signal failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t semaphore_timedwait_trap_impl(
	mach_port_name_t wait_name, unsigned int sec, clock_res_t nsec
) {
	int code = dserver_rpc_semaphore_timedwait(wait_name, sec, nsec);

	if (code < 0) {
		if (code == -LINUX_EINTR) {
			return KERN_ABORTED;
		}
		__simple_printf("semaphore_timedwait failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t semaphore_timedwait_signal_trap_impl(
	mach_port_name_t wait_name, mach_port_name_t signal_name, unsigned int sec,
	clock_res_t nsec
) {
	int code = dserver_rpc_semaphore_timedwait_signal(wait_name, signal_name, sec, nsec);

	if (code < 0) {
		if (code == -LINUX_EINTR) {
			return KERN_ABORTED;
		}
		__simple_printf("semaphore_timedwait_signal failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}
