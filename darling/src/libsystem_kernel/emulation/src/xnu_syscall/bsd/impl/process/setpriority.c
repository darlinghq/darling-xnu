#include <darling/emulation/xnu_syscall/bsd/impl/process/setpriority.h>

int sys_setpriority(int which, int who, int prio) {
	return LINUX_SYSCALL(__NR_setpriority, which, who, prio);

	// TODO: Errors
}
