#define __SYSCALL_32BIT_ARG_BYTES 36
#include "SYS.h"

#ifndef SYS_kevent_id
#error "SYS_kevent_id not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_kevent_id)
SYSCALL_NONAME(kevent_id, 8, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_kevent_id, kevent_id, 8, cerror_nocancel)
#endif

#endif

