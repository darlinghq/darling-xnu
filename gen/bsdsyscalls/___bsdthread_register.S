#define __SYSCALL_32BIT_ARG_BYTES 28
#include "SYS.h"

#ifndef SYS_bsdthread_register
#error "SYS_bsdthread_register not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___bsdthread_register)
SYSCALL_NONAME(bsdthread_register, 7, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___bsdthread_register, bsdthread_register, 7, cerror_nocancel)
#endif

#endif

