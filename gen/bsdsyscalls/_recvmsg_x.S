#define __SYSCALL_32BIT_ARG_BYTES 16
#include "SYS.h"

#ifndef SYS_recvmsg_x
#error "SYS_recvmsg_x not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_recvmsg_x)
SYSCALL_NONAME(recvmsg_x, 4, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_recvmsg_x, recvmsg_x, 4, cerror_nocancel)
#endif

#endif

