#define __SYSCALL_32BIT_ARG_BYTES 12
#include "SYS.h"

#ifndef SYS_write
#error "SYS_write not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_write)
ZERO_EXTEND(2)
SYSCALL_NONAME(write, 3, cerror)
ret
#else
#if defined(__arm64__) || defined(__x86_64__)
__SYSCALL2(_write, write, 3, cerror)
#else
__SYSCALL2(___write, write, 3, cerror)
#endif

#endif

#if defined(__i386__)
	.globl	_write$UNIX2003
	.set	_write$UNIX2003, ___write
#endif

#if defined(__ppc__)
	.globl	_write$UNIX2003
	.set	_write$UNIX2003, ___write
#endif

