#ifndef LINUX_PTRACE_H
#define LINUX_PTRACE_H

#define XNU_PT_PTRACE_ME 0
#define XNU_PT_CONTINUE 7
#define XNU_PT_KILL 8
#define XNU_PT_ATTACH 10
#define XNU_PT_ATTACHEXC 14
#define XNU_PT_DETACH 11
#define XNU_PT_SIGEXC 12
#define XNU_PT_THUPDATE 13

long sys_ptrace(int request, int pid, void* addr, int data);

#endif // LINUX_PTRACE_H
