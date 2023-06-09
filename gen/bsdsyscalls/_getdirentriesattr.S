#define __SYSCALL_32BIT_ARG_BYTES 32
#include "SYS.h"

#ifndef SYS_getdirentriesattr
#error "SYS_getdirentriesattr not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_getdirentriesattr)
ZERO_EXTEND(7)
ZERO_EXTEND(3)
SYSCALL_NONAME(getdirentriesattr, 8, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_getdirentriesattr, getdirentriesattr, 8, cerror_nocancel)
#endif

#endif

