#define __SYSCALL_32BIT_ARG_BYTES 20
#include "SYS.h"

#ifndef SYS_posix_spawn
#error "SYS_posix_spawn not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___posix_spawn)
SYSCALL_NONAME(posix_spawn, 5, cerror)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___posix_spawn, posix_spawn, 5, cerror)
#endif

#endif

