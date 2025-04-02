#ifndef DARLING_BSD_EMULATION_EXECVE_H
#define DARLING_BSD_EMULATION_EXECVE_H

long sys_execve(const char* fname, const char** argvp, const char** envp);

#endif // DARLING_BSD_EMULATION_EXECVE_H
