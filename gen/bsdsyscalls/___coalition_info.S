#define __SYSCALL_32BIT_ARG_BYTES 16
#include "SYS.h"

#ifndef SYS_coalition_info
#error "SYS_coalition_info not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___coalition_info)
SYSCALL_NONAME(coalition_info, 4, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___coalition_info, coalition_info, 4, cerror_nocancel)
#endif

#endif

