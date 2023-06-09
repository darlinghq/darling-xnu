#define __SYSCALL_32BIT_ARG_BYTES 16
#include "SYS.h"

#ifndef SYS_faccessat
#error "SYS_faccessat not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_faccessat)
SYSCALL_NONAME(faccessat, 4, cerror)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_faccessat, faccessat, 4, cerror)
#endif

#endif

