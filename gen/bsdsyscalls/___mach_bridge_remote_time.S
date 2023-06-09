#define __SYSCALL_32BIT_ARG_BYTES 8
#include "SYS.h"

#ifndef SYS___mach_bridge_remote_time
#error "SYS___mach_bridge_remote_time not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(___mach_bridge_remote_time)
SYSCALL_NONAME(__mach_bridge_remote_time, 1, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(___mach_bridge_remote_time, __mach_bridge_remote_time, 1, cerror_nocancel)
#endif

#endif

