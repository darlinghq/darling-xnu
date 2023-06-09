#define __SYSCALL_32BIT_ARG_BYTES 4
#include "SYS.h"

#ifndef SYS_ntp_adjtime
#error "SYS_ntp_adjtime not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_ntp_adjtime)
SYSCALL_NONAME(ntp_adjtime, 1, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_ntp_adjtime, ntp_adjtime, 1, cerror_nocancel)
#endif

#endif

