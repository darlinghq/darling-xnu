#define __SYSCALL_32BIT_ARG_BYTES 20
#include "SYS.h"

#ifndef SYS_pread
#error "SYS_pread not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_pread)
ZERO_EXTEND(2)
SYSCALL_NONAME(pread, 4, cerror)
ret
#else
#if defined(__arm64__) || defined(__x86_64__)
__SYSCALL2(_pread, pread, 4, cerror)
#else
__SYSCALL2(___pread, pread, 4, cerror)
#endif

#endif

#if defined(__i386__)
	.globl	_pread$UNIX2003
	.set	_pread$UNIX2003, ___pread
#endif

#if defined(__ppc__)
	.globl	_pread$UNIX2003
	.set	_pread$UNIX2003, ___pread
#endif

