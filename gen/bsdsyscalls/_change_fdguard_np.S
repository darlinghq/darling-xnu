#define __SYSCALL_32BIT_ARG_BYTES 24
#include "SYS.h"

#ifndef SYS_change_fdguard_np
#error "SYS_change_fdguard_np not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_change_fdguard_np)
SYSCALL_NONAME(change_fdguard_np, 6, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_change_fdguard_np, change_fdguard_np, 6, cerror_nocancel)
#endif

#endif

