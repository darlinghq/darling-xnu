#ifndef DARLING_BSD_EMULATION_SIGPROCMASK_H
#define DARLING_BSD_EMULATION_SIGPROCMASK_H

#include <emulation/conversion/methods/signal.h>

long sys_sigprocmask(int how, const bsd_sigset_t* set, bsd_sigset_t* oldset);

#endif // DARLING_BSD_EMULATION_SIGPROCMASK_H
