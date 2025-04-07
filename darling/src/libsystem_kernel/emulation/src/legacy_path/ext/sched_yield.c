#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

void __linux_sched_yield(void)
{
	LINUX_SYSCALL0(__NR_sched_yield);
}

