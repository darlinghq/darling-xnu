#include <darling/emulation/xnu_syscall/bsd/impl/misc/syscall.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/bsd_syscall_table.h>

long sys_syscall(int no, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
	long (*ptr)();

	if (no <= 0 || no >= sizeof(__bsd_syscall_table)/sizeof(__bsd_syscall_table[0]))
		return __unknown_syscall(no);

	ptr = (long(*)()) __bsd_syscall_table[no];
	if (!ptr)
		return __unknown_syscall(no);

	return ptr(a1, a2, a3, a4, a5, a6);
}
