#define __SYSCALL_32BIT_ARG_BYTES 28
#include "SYS.h"

#ifndef SYS_mmap
#error "SYS_mmap not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___mmap)
ZERO_EXTEND(1)
SYSCALL_NONAME(mmap, 6, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___mmap, mmap, 6, cerror_nocancel)
#endif

#endif

#if defined(__i386__)
	.globl	_mmap
	.set	_mmap, ___mmap
#endif

#if defined(__ppc__)
	.globl	_mmap
	.set	_mmap, ___mmap
#endif

