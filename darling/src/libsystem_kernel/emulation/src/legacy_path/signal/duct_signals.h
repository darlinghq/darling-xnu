#ifndef LINUX_DUCT_SIGNALS_H
#define LINUX_DUCT_SIGNALS_H
#define __sigset_t_defined
#include <sys/signal.h>

#define LINUX_NSIG 32
#define LINUX_SIGHUP 1
#define LINUX_SIGINT 2
#define LINUX_SIGQUIT 3
#define LINUX_SIGILL 4
#define LINUX_SIGTRAP 5
#define LINUX_SIGABRT 6
#define LINUX_SIGIOT 6
#define LINUX_SIGBUS 7
#define LINUX_SIGFPE 8
#define LINUX_SIGKILL 9
#define LINUX_SIGUSR1 10
#define LINUX_SIGSEGV 11
#define LINUX_SIGUSR2 12
#define LINUX_SIGPIPE 13
#define LINUX_SIGALRM 14
#define LINUX_SIGTERM 15
#define LINUX_SIGSTKFLT 16
#define LINUX_SIGCHLD 17
#define LINUX_SIGCONT 18
#define LINUX_SIGSTOP 19
#define LINUX_SIGTSTP 20
#define LINUX_SIGTTIN 21
#define LINUX_SIGTTOU 22
#define LINUX_SIGURG 23
#define LINUX_SIGXCPU 24
#define LINUX_SIGXFSZ 25
#define LINUX_SIGVTALRM 26
#define LINUX_SIGPROF 27
#define LINUX_SIGWINCH 28
#define LINUX_SIGIO 29
#define LINUX_SIGLOST 29
#define LINUX_SIGPWR 30
#define LINUX_SIGSYS 31
#define LINUX_SIGUNUSED 31
#define LINUX_SA_NOCLDSTOP 0x00000001u
#define LINUX_SA_NOCLDWAIT 0x00000002u
#define LINUX_SA_SIGINFO 0x00000004u
#define LINUX_SA_ONSTACK 0x08000000u
#define LINUX_SA_RESTART 0x10000000u
#define LINUX_SA_NODEFER 0x40000000u
#define LINUX_SA_RESETHAND 0x80000000u
#define LINUX_SA_RESTORER 0x04000000
#define LINUX_MINSIGSTKSZ 2048
#define LINUX_SIGSTKSZ 8192

typedef unsigned long long linux_sigset_t;

#ifndef __sigset_t_defined
typedef unsigned int sigset_t;
#endif

int signum_linux_to_bsd(int signum);
int signum_bsd_to_linux(int signum);
void sigset_linux_to_bsd(const linux_sigset_t* linux, sigset_t* bsd);
void sigset_bsd_to_linux(const sigset_t* bsd, linux_sigset_t* linux);

#endif

