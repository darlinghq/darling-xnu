#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_SIGNAL_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_SIGNAL_H

#define BSD_SIGHUP  1
#define BSD_SIGINT  2
#define BSD_SIGQUIT 3
#define BSD_SIGILL  4
#define BSD_SIGTRAP 5
#define BSD_SIGABRT 6
#define BSD_SIGFPE  8
#define BSD_SIGKILL 9
#define BSD_SIGBUS  10
#define BSD_SIGSEGV 11
#define BSD_SIGSYS  12
#define BSD_SIGPIPE 13
#define BSD_SIGALRM 14
#define BSD_SIGTERM 15
#define BSD_SIGURG  16
#define BSD_SIGSTOP 17
#define BSD_SIGTSTP 18
#define BSD_SIGCONT 19
#define BSD_SIGCHLD 20
#define BSD_SIGTTIN 21
#define BSD_SIGTTOU 22
#define BSD_SIGIO   23
#define BSD_SIGXCPU 24
#define BSD_SIGXFSZ 25
#define BSD_SIGVTALRM 26
#define BSD_SIGPROF 27
#define BSD_SIGWINCH 28
#define BSD_SIGINFO 29
#define BSD_SIGUSR1 30
#define BSD_SIGUSR2 31

#define BSD_SIG_DFL         (void (*)(int))0
#define BSD_SIG_IGN         (void (*)(int))1
#define BSD_SIG_HOLD        (void (*)(int))5
#define BSD_SIG_ERR         ((void (*)(int))-1)

#include <emulation/conversion/variables/bsd/sys/_types/_sigset_t.h>

#define BSD_SA_ONSTACK      0x0001
#define BSD_SA_RESTART      0x0002
#define BSD_SA_RESETHAND    0x0004
#define BSD_SA_NOCLDSTOP    0x0008
#define BSD_SA_NODEFER      0x0010
#define BSD_SA_NOCLDWAIT    0x0020
#define BSD_SA_SIGINFO      0x0040

#define BSD_SIG_BLOCK 1
#define BSD_SIG_UNBLOCK 2
#define BSD_SIG_SETMASK 3

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_SIGNAL_H
