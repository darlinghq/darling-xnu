#define __SYSCALL_32BIT_ARG_BYTES 20
#include "SYS.h"

#ifndef SYS_ulock_wait
#error "SYS_ulock_wait not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

/* Hand-written so the stub can honour ULF_NO_ERRNO; see ULOCK_SYSCALL in SYS.h. */
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__)
ULOCK_SYSCALL(___ulock_wait, ulock_wait, 4, cerror_nocancel)
#endif
