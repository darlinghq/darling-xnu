#define __SYSCALL_32BIT_ARG_BYTES 12
#include "SYS.h"

#ifndef SYS_fcntl
#error "SYS_fcntl not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___fcntl)
SIGN_EXTEND(2)
SYSCALL_NONAME(fcntl, 3, cerror)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___fcntl, fcntl, 3, cerror)
#endif

#endif

#if defined(__i386__)
	.globl	_fcntl$UNIX2003
	.set	_fcntl$UNIX2003, ___fcntl
#endif

#if defined(__ppc__)
	.globl	_fcntl$UNIX2003
	.set	_fcntl$UNIX2003, ___fcntl
#endif

