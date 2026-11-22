#include <darling/emulation/xnu_syscall/bsd/impl/misc/syscall.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/bsd_syscall_table.h>

// Syscall numbers carry a class in bits 24 and up. Class 7 is not a kernel class:
// a dyld that sees the commpage report a translated process (kIsTranslated, which
// Darling's mldr sets for emulated x86_64) issues these through syscall() to ask
// the binary translator for an ahead-of-time translation of the image it maps
// (numbers 0x7000001 and 0x7000004 so far), and maps the plain image when the
// call fails. Darling has no such translations: answer "not available" quietly
// instead of reporting each one as an unimplemented syscall.
#define SYSCALL_CLASS_TRANSLATOR 7

long sys_syscall(int no, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6)
{
	long (*ptr)();

	if ((no >> 24) == SYSCALL_CLASS_TRANSLATOR)
		return -ENOSYS;

	if (no <= 0 || no >= sizeof(__bsd_syscall_table)/sizeof(__bsd_syscall_table[0]))
		return __unknown_syscall(no);

	ptr = (long(*)()) __bsd_syscall_table[no];
	if (!ptr)
		return __unknown_syscall(no);

	return ptr(a1, a2, a3, a4, a5, a6);
}
