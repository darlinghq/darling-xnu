#define __SYSCALL_32BIT_ARG_BYTES 32
#include "SYS.h"

#ifndef SYS_os_fault_with_payload
#error "SYS_os_fault_with_payload not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_os_fault_with_payload)
SYSCALL_NONAME(os_fault_with_payload, 6, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_os_fault_with_payload, os_fault_with_payload, 6, cerror_nocancel)
#endif

#endif

