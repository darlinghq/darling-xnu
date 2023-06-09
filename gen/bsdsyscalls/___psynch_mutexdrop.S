#define __SYSCALL_32BIT_ARG_BYTES 24
#include "SYS.h"

#ifndef SYS_psynch_mutexdrop
#error "SYS_psynch_mutexdrop not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___psynch_mutexdrop)
SYSCALL_NONAME(psynch_mutexdrop, 5, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___psynch_mutexdrop, psynch_mutexdrop, 5, cerror_nocancel)
#endif

#endif

