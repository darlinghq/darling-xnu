#ifndef DARLING_BSD_SYSCALL_H
#define DARLING_BSD_SYSCALL_H

long __darling_bsd_syscall(int nr, ...);
long __darling_bsd_unknown_sysnum(int nr, ...);

#endif // DARLING_BSD_EMULATION_SYSCALL_H
