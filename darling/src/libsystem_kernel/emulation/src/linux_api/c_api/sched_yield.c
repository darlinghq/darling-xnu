#include <emulation/linux_api/c_api/sched_yield.h>

#include <emulation/linux_api/linux_syscall.h>

void __linux_sched_yield(void)
{
	LINUX_SYSCALL0(__NR_sched_yield);
}
