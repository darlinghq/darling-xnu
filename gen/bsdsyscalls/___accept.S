#define __SYSCALL_32BIT_ARG_BYTES 12
#include "SYS.h"

#ifndef SYS_accept
#error "SYS_accept not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___accept)
SYSCALL_NONAME(accept, 3, cerror)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___accept, accept, 3, cerror)
#endif

#endif

#if defined(__arm64__)
	.globl	_accept
	.set	_accept, ___accept
#endif

#if defined(__i386__)
	.globl	_accept$UNIX2003
	.set	_accept$UNIX2003, ___accept
#endif

#if defined(__x86_64__)
	.globl	_accept
	.set	_accept, ___accept
#endif

#if defined(__ppc__)
	.globl	_accept$UNIX2003
	.set	_accept$UNIX2003, ___accept
#endif

