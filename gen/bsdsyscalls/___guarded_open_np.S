#define __SYSCALL_32BIT_ARG_BYTES 20
#include "SYS.h"

#ifndef SYS_guarded_open_np
#error "SYS_guarded_open_np not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___guarded_open_np)
SYSCALL_NONAME(guarded_open_np, 5, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___guarded_open_np, guarded_open_np, 5, cerror_nocancel)
#endif

#endif

