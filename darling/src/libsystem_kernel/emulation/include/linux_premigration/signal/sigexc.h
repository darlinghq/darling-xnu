#ifndef _SIGEXC_H
#define _SIGEXC_H

#include <stdbool.h>

#include "rtsig.h"
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigaction.h>

// NOTE: Keep these definitions up to date with lkm/darling/binfmt.c!
// Uses one of the below magic values to toggle the debugging state
#define SIGNAL_SIGEXC_SUSPEND	LINUX_SIGRTMIN
#define SIGNAL_S2C (LINUX_SIGRTMIN + 1)

void sigexc_setup(void);

// for PT_SIGEXC to handle this operation synchronously
void darling_sigexc_self(void);
void sigexc_handler(int linux_signum, struct linux_siginfo* info, struct linux_ucontext* ctxt);

// Whether sigexc_handler is the Linux handler of this signal once the application has called
// sigaction() for it (as opposed to signals the kernel does not allow handling, and SIGCHLD,
// SIGTTOU and SIGTTIN, whose Linux disposition sigaction() sets itself, because the kernel acts
// on them being ignored: an ignored SIGTTOU lets a background process group call tcsetpgrp(),
// an ignored SIGTTIN makes its read() from the terminal fail with EIO instead of looping).
bool sigexc_handles_signal(int linux_signum);

// Applies the application's disposition of a signal to its sigexc_handler: whether the kernel restarts
// blocking syscalls the handler interrupted (SA_RESTART) and whether it runs the handler on the
// alternate stack (SA_ONSTACK). bsd_flags are the Darwin sigaction() flags; without a handler (SIG_DFL,
// SIG_IGN) they do not matter, as no application code runs and no syscall is interrupted.
void sigexc_set_disposition(int linux_signum, bool has_handler, int bsd_flags);

// Registers an eventfd that counts each delivery of the signal (for kqueue
// signal filters); returns 0 or a negative BSD errno.
int sigexc_kqueue_watch(int linux_signum, int fd);
void sigexc_kqueue_unwatch(int linux_signum, int fd);

// How many guest signal handlers the calling thread has run so far. A blocking Linux syscall that
// returns EINTR was interrupted by a signal the guest sees only if this changed over the call; other
// interruptions (signals that are ignored or dropped, darlingserver's own) are not visible to the guest.
// Only valid once libpthread has registered (bsdthread_registered()).
unsigned long sigexc_handlers_run(void);

// Tells darlingserver that the calling thread has entered a signal handler (dserver_rpc_interrupt_enter()),
// with every other signal that could run a handler blocked until the server has answered. `transparent` is
// true for the signals of Darling's own machinery (thread suspension, server-to-client calls), which the
// application never sees: a semaphore wait they interrupt is made again instead of ending with
// KERN_ABORTED, as in XNU.
int sigexc_interrupt_enter(bool transparent);

// Gives back the alternate stack Darling took away for a handler that then left by longjmp(), when the
// signal of ctxt interrupted the thread above where that handler started. Every Linux handler that runs
// application handlers calls it first.
void sigexc_unpark_abandoned_altstack(struct linux_ucontext* ctxt);

// Runs the application's handler of a signal (handler_linux_to_bsd()) with the signal mask given, for
// sigexc_handler and for the handler sigaction() installs for SIGCHLD, SIGTTOU and SIGTTIN. Where the
// kernel ran the caller on an alternate stack the application's handler is not to run on (Darling's
// own, or any for a handler without SA_ONSTACK), it runs on the interrupted stack, below its red zone.
void sigexc_run_app_handler(int linux_signum, struct linux_siginfo* info, struct linux_ucontext* ctxt,
		const linux_sigset_t* mask);

// Called by sigaltstack() before it replaces the calling thread's alternate stack, so that Darling's
// own stays known (see sigexc_thread_state).
void sigexc_altstack_will_change(void);

// Turns the calling thread's alternate stack, as Linux reports it, into the one the application installed:
// Darling's own is reported as none (SS_DISABLE, no address or size, as for a thread that never installed
// one), and the application's stack, while Darling has taken it away for a handler running on the
// interrupted stack, as still installed.
struct linux_stack;
void sigexc_altstack_app_view(struct linux_stack* stack);

void sigexc_thread_setup(void);
void sigexc_thread_exit(void);

#endif // _SIGEXC_H
