#define __SYSCALL_32BIT_ARG_BYTES 20
#include "SYS.h"

#ifndef SYS_fchmod_extended
#error "SYS_fchmod_extended not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___fchmod_extended)
SYSCALL_NONAME(fchmod_extended, 5, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___fchmod_extended, fchmod_extended, 5, cerror_nocancel)
#endif

#endif

