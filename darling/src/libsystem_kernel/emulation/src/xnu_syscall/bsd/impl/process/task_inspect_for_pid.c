#include <darling/emulation/xnu_syscall/bsd/impl/process/task_inspect_for_pid.h>

#include <darling/emulation/common/simple.h>

#include <darlingserver/rpc.h>

long sys_task_inspect_for_pid(unsigned int target_tport, int pid, unsigned int* t)
{
	int code = dserver_rpc_task_inspect_for_pid(target_tport, pid, t);

	if (code < 0) {
		__simple_printf("task_inspect_for_pid failed (internally): %d\n", code);
		__simple_abort();
	}

	return -code;
}
