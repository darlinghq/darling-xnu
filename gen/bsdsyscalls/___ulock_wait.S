#define __SYSCALL_32BIT_ARG_BYTES 20
#include "SYS.h"

#ifndef SYS_ulock_wait
#error "SYS_ulock_wait not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___ulock_wait)
SYSCALL_NONAME(ulock_wait, 4, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___ulock_wait, ulock_wait, 4, cerror_nocancel)
#endif

#endif

