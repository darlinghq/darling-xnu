#define __SYSCALL_32BIT_ARG_BYTES 8
#include "SYS.h"

#ifndef SYS_munmap
#error "SYS_munmap not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___munmap)
ZERO_EXTEND(1)
SYSCALL_NONAME(munmap, 2, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___munmap, munmap, 2, cerror_nocancel)
#endif

#endif

