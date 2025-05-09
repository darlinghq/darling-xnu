#ifndef LINUX_SIGACTION_H
#define LINUX_SIGACTION_H

#include <darling/emulation/conversion/signal/sigaction.h>

long sys_sigaction(int signum, const struct bsd___sigaction* nsa, struct bsd_sigaction* osa);

extern bsd_sig_handler* sig_handlers[32];
extern int sig_flags[32];
extern unsigned int sig_masks[32];

#endif // LINUX_SIGACTION_H
