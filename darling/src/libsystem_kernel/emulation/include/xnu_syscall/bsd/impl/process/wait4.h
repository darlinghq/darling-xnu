#ifndef LINUX_WAIT4_H
#define LINUX_WAIT4_H

long sys_wait4(int pid, int* status, int options, void* rusage);
long sys_wait4_nocancel(int pid, int* status, int options, void* rusage);

#define DARLING_WAIT_NORESTART 0x20000

#endif // LINUX_WAIT4_H
