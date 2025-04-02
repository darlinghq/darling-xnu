#ifndef DARLING_EMULATION_CONVERSION_SIGACTION_H
#define DARLING_EMULATION_CONVERSION_SIGACTION_H

#include <emulation/conversion/methods/signal.h>
#include <emulation/conversion/variables/linux/asm-generic/siginfo.h>
#include <emulation/xnu_syscall/bsd/impl/signal/sigaltstack.h>

void handler_linux_to_bsd(int linux_signum, struct linux_siginfo* info, void* ctxt);

extern int sig_flags[32];
extern unsigned int sig_masks[32];

extern int sigflags_bsd_to_linux(int flags);
extern int sigflags_linux_to_bsd(int flags);

#endif // DARLING_EMULATION_CONVERSION_SIGACTION_H
