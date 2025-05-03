#ifndef LINUX_SIGPROCMASK_H
#define LINUX_SIGPROCMASK_H

#include <darling/emulation/conversion/signal/duct_signals.h>

long sys_sigprocmask(int how, const sigset_t* set, sigset_t* oldset);

#endif // LINUX_SIGPROCMASK_H
