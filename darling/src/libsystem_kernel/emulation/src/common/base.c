#define BUILDING_BASE_C
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/xnu_syscall/bsd/bsd_syscall_table.h>

long __unknown_syscall(int nr, ...)
{
	__simple_printf("Unimplemented syscall (%d)\n", nr);
	return -ENOSYS;
}

long __unknown_syscall_machdep(int nr, ...)
{
	__simple_printf("Unimplemented machdep trap (%d)\n", nr);
	return -ENOSYS;
}

VISIBLE
int __linux_syscall(int nr, long a1, long a2, long a3, long a4, long a5, long a6)
{
	return linux_syscall(a1, a2, a3, a4, a5, a6, nr);
}

#ifdef __TESTING
void _start()
{
#ifdef __x86_64__
	__asm__ ("andq    $-16, %rsp");
#elif defined(__i386__
	__asm__ ("andl    $-16, %esp");
#endif
	__simple_printf("Hello world\n");
	LINUX_SYSCALL1(__NR_exit, 0);
}
#endif
