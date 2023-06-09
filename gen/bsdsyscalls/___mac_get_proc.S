#define __SYSCALL_32BIT_ARG_BYTES 4
#include "SYS.h"

#ifndef SYS___mac_get_proc
#error "SYS___mac_get_proc not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___mac_get_proc)
SYSCALL_NONAME(__mac_get_proc, 1, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___mac_get_proc, __mac_get_proc, 1, cerror_nocancel)
#endif

#endif

