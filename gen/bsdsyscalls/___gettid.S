#define __SYSCALL_32BIT_ARG_BYTES 8
#include "SYS.h"

#ifndef SYS_gettid
#error "SYS_gettid not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___gettid)
SYSCALL_NONAME(gettid, 2, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___gettid, gettid, 2, cerror_nocancel)
#endif

#endif

#if defined(__arm64__)
	.globl	_pthread_getugid_np
	.set	_pthread_getugid_np, ___gettid
#endif

#if defined(__i386__)
	.globl	_pthread_getugid_np
	.set	_pthread_getugid_np, ___gettid
#endif

#if defined(__x86_64__)
	.globl	_pthread_getugid_np
	.set	_pthread_getugid_np, ___gettid
#endif

#if defined(__ppc__)
	.globl	_pthread_getugid_np
	.set	_pthread_getugid_np, ___gettid
#endif

