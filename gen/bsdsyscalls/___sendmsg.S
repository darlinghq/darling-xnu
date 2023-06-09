#define __SYSCALL_32BIT_ARG_BYTES 12
#include "SYS.h"

#ifndef SYS_sendmsg
#error "SYS_sendmsg not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___sendmsg)
SYSCALL_NONAME(sendmsg, 3, cerror)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___sendmsg, sendmsg, 3, cerror)
#endif

#endif

#if defined(__arm64__)
	.globl	_sendmsg
	.set	_sendmsg, ___sendmsg
#endif

#if defined(__i386__)
	.globl	_sendmsg$UNIX2003
	.set	_sendmsg$UNIX2003, ___sendmsg
#endif

#if defined(__x86_64__)
	.globl	_sendmsg
	.set	_sendmsg, ___sendmsg
#endif

#if defined(__ppc__)
	.globl	_sendmsg$UNIX2003
	.set	_sendmsg$UNIX2003, ___sendmsg
#endif

