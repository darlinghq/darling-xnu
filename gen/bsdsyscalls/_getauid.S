#define __SYSCALL_32BIT_ARG_BYTES 4
#include "SYS.h"

#ifndef SYS_getauid
#error "SYS_getauid not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_getauid)
SYSCALL_NONAME(getauid, 1, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_getauid, getauid, 1, cerror_nocancel)
#endif

#endif

