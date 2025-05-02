#include <darling/emulation/xnu_syscall/bsd/impl/process/getpriority.h>

int sys_getpriority(int which, id_t who) {
	return LINUX_SYSCALL(__NR_getpriority, which, who);

	// TODO: Errors
}
