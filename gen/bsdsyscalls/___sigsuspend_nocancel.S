#define __SYSCALL_32BIT_ARG_BYTES 4
#include "SYS.h"

#ifndef SYS_sigsuspend_nocancel
#error "SYS_sigsuspend_nocancel not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___sigsuspend_nocancel)
SYSCALL_NONAME(sigsuspend_nocancel, 1, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___sigsuspend_nocancel, sigsuspend_nocancel, 1, cerror_nocancel)
#endif

#endif

