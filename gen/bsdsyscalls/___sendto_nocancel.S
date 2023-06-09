#define __SYSCALL_32BIT_ARG_BYTES 24
#include "SYS.h"

#ifndef SYS_sendto_nocancel
#error "SYS_sendto_nocancel not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___sendto_nocancel)
ZERO_EXTEND(2)
SYSCALL_NONAME(sendto_nocancel, 6, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___sendto_nocancel, sendto_nocancel, 6, cerror_nocancel)
#endif

#endif

#if defined(__arm64__)
	.globl	_sendto$NOCANCEL
	.set	_sendto$NOCANCEL, ___sendto_nocancel
#endif

#if defined(__i386__)
	.globl	_sendto$NOCANCEL$UNIX2003
	.set	_sendto$NOCANCEL$UNIX2003, ___sendto_nocancel
#endif

#if defined(__x86_64__)
	.globl	_sendto$NOCANCEL
	.set	_sendto$NOCANCEL, ___sendto_nocancel
#endif

#if defined(__ppc__)
	.globl	_sendto$NOCANCEL$UNIX2003
	.set	_sendto$NOCANCEL$UNIX2003, ___sendto_nocancel
#endif

