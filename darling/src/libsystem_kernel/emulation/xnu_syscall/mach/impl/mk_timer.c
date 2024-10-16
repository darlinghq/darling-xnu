#include "mk_timer.h"

#include <darlingserver/rpc.h>
#include <emulation/common/simple.h>

/*
 *	Obsolete interfaces.
 */

mach_port_name_t mk_timer_create_impl(void)
{
	unsigned int port_name;
	if (dserver_rpc_mk_timer_create(&port_name) < 0) {
		port_name = MACH_PORT_NULL;
	}
	return port_name;
}

kern_return_t mk_timer_destroy_impl(mach_port_name_t name)
{
	int code = dserver_rpc_mk_timer_destroy(name);

	if (code < 0) {
		__simple_printf("mk_timer_destroy failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t mk_timer_arm_impl(mach_port_name_t name, uint64_t expire_time)
{
	int code = dserver_rpc_mk_timer_arm(name, expire_time);

	if (code < 0) {
		__simple_printf("mk_timer_arm failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}

kern_return_t mk_timer_cancel_impl(mach_port_name_t name, uint64_t *result_time)
{
	int code = dserver_rpc_mk_timer_cancel(name, result_time);

	if (code < 0) {
		__simple_printf("mk_timer_cancel failed (internally): %d\n", code);
		__simple_abort();
	}

	return code;
}