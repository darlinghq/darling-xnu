// XNU BSD syscalls 496-498, the mach eventlink signal / wait "traps" (libsyscall mach_eventlink.c): done by
// darlingserver's XNU (ipc_eventlink.c), which writes the 64-bit result (count | error << 56) to `result`.
#include <darling/emulation/xnu_syscall/bsd/impl/mach/mach_eventlink.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darlingserver/rpc.h>

#include <mach/kern_return.h>

// mach/mach_eventlink_types.h: error in the top byte
#define EL_RESULT(kr) ((uint64_t)(kr) << 56)

static uint64_t finish(int code, uint64_t result) {
	if (code < 0) {
		if (code == -LINUX_EINTR)
			return EL_RESULT(KERN_ABORTED);
		__simple_printf("mach_eventlink call failed (internally): %d\n", code);
		__simple_abort();
	}
	if (code != KERN_SUCCESS)
		return EL_RESULT(code);
	return result;
}

uint64_t sys_mach_eventlink_signal(unsigned int eventlink_port, uint64_t signal_count) {
	uint64_t result = 0;
	return finish(dserver_rpc_mach_eventlink_signal(eventlink_port, signal_count, &result), result);
}

uint64_t sys_mach_eventlink_wait_until(unsigned int eventlink_port, uint64_t wait_count, uint64_t deadline,
		unsigned int clock_id, unsigned int option) {
	uint64_t result = 0;
	return finish(dserver_rpc_mach_eventlink_wait_until(eventlink_port, wait_count, deadline, clock_id, option, &result), result);
}

uint64_t sys_mach_eventlink_signal_wait_until(unsigned int eventlink_port, uint64_t wait_count, uint64_t signal_count,
		uint64_t deadline, unsigned int clock_id, unsigned int option) {
	uint64_t result = 0;
	return finish(dserver_rpc_mach_eventlink_signal_wait_until(eventlink_port, wait_count, signal_count, deadline, clock_id,
		option, &result), result);
}
