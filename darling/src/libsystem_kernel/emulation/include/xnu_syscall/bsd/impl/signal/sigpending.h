#ifndef LINUX_SIGPENDING_H
#define LINUX_SIGPENDING_H

#include <darling/emulation/conversion/signal/duct_signals.h>

long sys_sigpending(sigset_t set);

#endif // LINUX_SIGPENDING_H
