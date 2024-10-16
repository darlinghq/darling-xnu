#include "syscall.h"

#include <stddef.h>

#include <emulation/xnu_syscall/bsd/bsd_syscall.h>
#include <emulation/xnu_syscall/bsd/bsd_syscall_table.h>

long sys_syscall(int no, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
	long (*ptr)();

	if (no <= 0 || no >= sizeof(__bsd_syscall_table)/sizeof(__bsd_syscall_table[0]))
		return __darling_bsd_unknown_sysnum(no);

	ptr = (long(*)()) __bsd_syscall_table[no];
	if (!ptr)
		return __darling_bsd_unknown_sysnum(no);

	return ptr(a1, a2, a3, a4, a5, a6);
}