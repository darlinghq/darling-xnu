#ifndef DARLING_EMULATION_CONVERSION_SIGEXC_H
#define DARLING_EMULATION_CONVERSION_SIGEXC_H

#include <stdbool.h>

#include <emulation/conversion/signal/context.h>
#include <emulation/conversion/signal/sigaction.h>

#define SIGEXC_STACK_SIZE (48ULL * 1024ULL)

// NOTE: Keep these definitions up to date with lkm/darling/binfmt.c!
// Uses one of the below magic values to toggle the debugging state
#define SIGNAL_SIGEXC_SUSPEND	LINUX_SIGRTMIN
#define SIGNAL_S2C (LINUX_SIGRTMIN + 1)

void handle_rt_signal(int signum);
extern char sigexc_altstack[];

// for PT_SIGEXC to handle this operation synchronously
void sigexc_handler(int linux_signum, struct linux_siginfo* info, struct linux_ucontext* ctxt);

void sigexc_thread_setup(void);
void sigexc_thread_exit(void);

#endif // DARLING_EMULATION_CONVERSION_SIGEXC_H
