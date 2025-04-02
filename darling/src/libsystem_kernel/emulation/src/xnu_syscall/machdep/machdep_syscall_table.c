#include <emulation/xnu_syscall/machdep/machdep_syscall_table.h>

void* __machdep_syscall_table[10] = {
	[3] = sys_thread_set_tsd_base,
};

