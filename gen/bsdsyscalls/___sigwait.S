#define __SYSCALL_32BIT_ARG_BYTES 8
#include "SYS.h"

#ifndef SYS___sigwait
#error "SYS___sigwait not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___sigwait)
SYSCALL_NONAME(__sigwait, 2, cerror)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___sigwait, __sigwait, 2, cerror)
#endif

#endif

