#define __SYSCALL_32BIT_ARG_BYTES 12
#include "SYS.h"

#ifndef SYS_accept_nocancel
#error "SYS_accept_nocancel not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___accept_nocancel)
SYSCALL_NONAME(accept_nocancel, 3, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___accept_nocancel, accept_nocancel, 3, cerror_nocancel)
#endif

#endif

#if defined(__arm64__)
	.globl	_accept$NOCANCEL
	.set	_accept$NOCANCEL, ___accept_nocancel
#endif

#if defined(__i386__)
	.globl	_accept$NOCANCEL$UNIX2003
	.set	_accept$NOCANCEL$UNIX2003, ___accept_nocancel
#endif

#if defined(__x86_64__)
	.globl	_accept$NOCANCEL
	.set	_accept$NOCANCEL, ___accept_nocancel
#endif

#if defined(__ppc__)
	.globl	_accept$NOCANCEL$UNIX2003
	.set	_accept$NOCANCEL$UNIX2003, ___accept_nocancel
#endif

