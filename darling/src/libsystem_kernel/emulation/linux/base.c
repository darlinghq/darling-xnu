#define BUILDING_BASE_C
#include "base.h"
#include <linux-syscalls/linux.h>
#include "simple.h"
#include "duct_errno.h"
#include "syscalls.h"

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
int __linux_syscall_6args(int nr, long a1, long a2, long a3, long a4, long a5, long a6)
{
	return linux_syscall(a1, a2, a3, a4, a5, a6, nr);
}

VISIBLE
int __linux_syscall_noargs(int nr)
{
	return __linux_syscall_6args(nr, 0, 0, 0, 0, 0, 0);
}

VISIBLE
int __linux_syscall_1arg(int nr, long a1)
{
	return __linux_syscall_6args(nr, a1, 0, 0, 0, 0, 0);
}

VISIBLE
int __linux_syscall_2args(int nr, long a1, long a2)
{
	return __linux_syscall_6args(nr, a1, a2, 0, 0, 0, 0);
}

VISIBLE
int __linux_syscall_3args(int nr, long a1, long a2, long a3)
{
	return __linux_syscall_6args(nr, a1, a2, a3, 0, 0, 0);
}

VISIBLE
int __linux_syscall_4args(int nr, long a1, long a2, long a3, long a4)
{
	return __linux_syscall_6args(nr, a1, a2, a3, a4, 0, 0);
}

VISIBLE
int __linux_syscall_5args(int nr, long a1, long a2, long a3, long a4, long a5)
{
	return __linux_syscall_6args(nr, a1, a2, a3, a4, a5, 0);
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

