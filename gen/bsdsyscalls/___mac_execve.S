#define __SYSCALL_32BIT_ARG_BYTES 16
#include "SYS.h"

#ifndef SYS___mac_execve
#error "SYS___mac_execve not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___mac_execve)
SYSCALL_NONAME(__mac_execve, 4, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___mac_execve, __mac_execve, 4, cerror_nocancel)
#endif

#endif

#if defined(__arm64__)
	.globl	___sandbox_me
	.set	___sandbox_me, ___mac_execve
#endif

#if defined(__i386__)
	.globl	___sandbox_me
	.set	___sandbox_me, ___mac_execve
#endif

#if defined(__x86_64__)
	.globl	___sandbox_me
	.set	___sandbox_me, ___mac_execve
#endif

#if defined(__ppc__)
	.globl	___sandbox_me
	.set	___sandbox_me, ___mac_execve
#endif

