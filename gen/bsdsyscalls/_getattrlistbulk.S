#define __SYSCALL_32BIT_ARG_BYTES 24
#include "SYS.h"

#ifndef SYS_getattrlistbulk
#error "SYS_getattrlistbulk not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_getattrlistbulk)
ZERO_EXTEND(3)
SYSCALL_NONAME(getattrlistbulk, 5, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_getattrlistbulk, getattrlistbulk, 5, cerror_nocancel)
#endif

#endif

