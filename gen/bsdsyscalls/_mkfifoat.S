#define __SYSCALL_32BIT_ARG_BYTES 12
#include "SYS.h"

#ifndef SYS_mkfifoat
#error "SYS_mkfifoat not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_mkfifoat)
SYSCALL_NONAME(mkfifoat, 3, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_mkfifoat, mkfifoat, 3, cerror_nocancel)
#endif

#endif
