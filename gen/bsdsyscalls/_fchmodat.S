#define __SYSCALL_32BIT_ARG_BYTES 16
#include "SYS.h"

#ifndef SYS_fchmodat
#error "SYS_fchmodat not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_fchmodat)
SYSCALL_NONAME(fchmodat, 4, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_fchmodat, fchmodat, 4, cerror_nocancel)
#endif

#endif

