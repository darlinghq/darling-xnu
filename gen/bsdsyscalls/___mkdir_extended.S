#define __SYSCALL_32BIT_ARG_BYTES 20
#include "SYS.h"

#ifndef SYS_mkdir_extended
#error "SYS_mkdir_extended not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___mkdir_extended)
SYSCALL_NONAME(mkdir_extended, 5, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___mkdir_extended, mkdir_extended, 5, cerror_nocancel)
#endif

#endif

