#define __SYSCALL_32BIT_ARG_BYTES 16
#include "SYS.h"

#ifndef SYS_proc_uuid_policy
#error "SYS_proc_uuid_policy not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_proc_uuid_policy)
ZERO_EXTEND(2)
SYSCALL_NONAME(proc_uuid_policy, 4, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_proc_uuid_policy, proc_uuid_policy, 4, cerror_nocancel)
#endif

#endif

