#define __SYSCALL_32BIT_ARG_BYTES 20
#include "SYS.h"

#ifndef SYS___old_semwait_signal
#error "SYS___old_semwait_signal not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___old_semwait_signal)
SYSCALL_NONAME(__old_semwait_signal, 5, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___old_semwait_signal, __old_semwait_signal, 5, cerror_nocancel)
#endif

#endif

