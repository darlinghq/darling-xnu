#define __SYSCALL_32BIT_ARG_BYTES 16
#include "SYS.h"

#ifndef SYS_waitid
#error "SYS_waitid not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_waitid)
SYSCALL_NONAME(waitid, 4, cerror)
ret
#else
#if defined(__arm64__) || defined(__x86_64__)
__SYSCALL2(_waitid, waitid, 4, cerror)
#else
__SYSCALL2(___waitid, waitid, 4, cerror)
#endif

#endif

#if defined(__i386__)
	.globl	_waitid$UNIX2003
	.set	_waitid$UNIX2003, ___waitid
#endif

#if defined(__ppc__)
	.globl	_waitid$UNIX2003
	.set	_waitid$UNIX2003, ___waitid
#endif

