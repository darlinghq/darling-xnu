#define __SYSCALL_32BIT_ARG_BYTES 32
#include "SYS.h"

#ifndef SYS_ulock_wait2
#error "SYS_ulock_wait2 not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

/* Hand-written so the stub can honour ULF_NO_ERRNO; see ULOCK_SYSCALL in SYS.h. */
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__)
ULOCK_SYSCALL(___ulock_wait2, ulock_wait2, 5, cerror_nocancel)
#endif
