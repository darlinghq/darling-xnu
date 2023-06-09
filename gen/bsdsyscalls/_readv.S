#define __SYSCALL_32BIT_ARG_BYTES 12
#include "SYS.h"

#ifndef SYS_readv
#error "SYS_readv not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_readv)
SYSCALL_NONAME(readv, 3, cerror)
ret
#else
#if defined(__arm64__) || defined(__x86_64__)
__SYSCALL2(_readv, readv, 3, cerror)
#else
__SYSCALL2(___readv, readv, 3, cerror)
#endif

#endif

#if defined(__i386__)
	.globl	_readv$UNIX2003
	.set	_readv$UNIX2003, ___readv
#endif

#if defined(__ppc__)
	.globl	_readv$UNIX2003
	.set	_readv$UNIX2003, ___readv
#endif

