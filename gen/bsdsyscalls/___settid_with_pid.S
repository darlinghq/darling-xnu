#define __SYSCALL_32BIT_ARG_BYTES 8
#include "SYS.h"

#ifndef SYS_settid_with_pid
#error "SYS_settid_with_pid not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___settid_with_pid)
SYSCALL_NONAME(settid_with_pid, 2, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___settid_with_pid, settid_with_pid, 2, cerror_nocancel)
#endif

#endif

