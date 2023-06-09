#define __SYSCALL_32BIT_ARG_BYTES 4
#include "SYS.h"

#ifndef SYS_fileport_makefd
#error "SYS_fileport_makefd not defined. The header files libsyscall is building against do not match syscalls.master."
#endif

#if defined(__arm64__)
MI_ENTRY_POINT(_fileport_makefd)
SYSCALL_NONAME(fileport_makefd, 1, cerror_nocancel)
ret
#else
#if defined(__arm64__) || defined(__i386__) || defined(__x86_64__) || defined(__ppc__)
__SYSCALL2(_fileport_makefd, fileport_makefd, 1, cerror_nocancel)
#endif

#endif

