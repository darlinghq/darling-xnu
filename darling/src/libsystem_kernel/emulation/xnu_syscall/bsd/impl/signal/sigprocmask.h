#ifndef DARLING_BSD_EMULATION_SIGPROCMASK_H
#define DARLING_BSD_EMULATION_SIGPROCMASK_H

#include <emulation/conversion/signal.h>

long sys_sigprocmask(int how, const sigset_t* set, sigset_t* oldset);

#endif // DARLING_BSD_EMULATION_SIGPROCMASK_H
