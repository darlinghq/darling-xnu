#define __SYSCALL_32BIT_ARG_BYTES 12
#include "SYS.h"

#ifndef SYS_open_nocancel
#error "SYS_open_nocancel not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___open_nocancel)
SYSCALL_NONAME(open_nocancel, 3, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___open_nocancel, open_nocancel, 3, cerror_nocancel)
#endif

#endif

#if defined(__i386__)
	.globl	_open$NOCANCEL$UNIX2003
	.set	_open$NOCANCEL$UNIX2003, ___open_nocancel
#endif

#if defined(__ppc__)
	.globl	_open$NOCANCEL$UNIX2003
	.set	_open$NOCANCEL$UNIX2003, ___open_nocancel
#endif

