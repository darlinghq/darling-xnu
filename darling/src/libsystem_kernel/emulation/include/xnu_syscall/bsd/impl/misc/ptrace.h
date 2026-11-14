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
#define XNU_PT_DENY_ATTACH 31

long sys_ptrace(int request, int pid, void* addr, int data);

// Processes this process attached to with PT_ATTACH/PT_ATTACHEXC and has not detached from.
// Like the effective parent XNU makes of a tracer, they can be waited for with wait4()
// even though they are not children in the Linux sense.
//
// Waits for the exit of a traced process matching `pid` (a process ID, or -1 for any).
// Returns the process ID and sets *linux_status (Linux wait status format) once it has exited,
// 0 if `nohang` and none has exited yet, or a negative Linux errno (-ECHILD if no traced
// process matches).
long ptrace_wait_tracee(int pid, int* linux_status, int nohang);

#endif // LINUX_PTRACE_H
