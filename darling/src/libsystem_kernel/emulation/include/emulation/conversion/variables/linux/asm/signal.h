#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_SIGNAL_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_SIGNAL_H

#include <emulation/conversion/variables/linux/linux/types.h>

struct linux_siginfo;

#define LINUX_NSIG 32
typedef unsigned long linux_sigset_t;

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

// Modified to match userspace
// TODO: Hardcoding the SIGRTMIN is dangerous
#define LINUX_SIGRTMIN 34 

#define LINUX_SA_RESTORER 0x04000000

#define LINUX_MINSIGSTKSZ 2048
#define LINUX_SIGSTKSZ 8192

#include <emulation/conversion/variables/linux/asm-generic/signal-defs.h>

// Added sigaction method type to avoid build errors
typedef void (*linux_sa_sigaction_t)(int, struct linux_siginfo *, void *);

#if defined(__i386__)

struct linux_sigaction {
	union {
	  __linux_sighandler_t _sa_handler;
	  linux_sa_sigaction_t _sa_sigaction;
	} _u;
	linux_sigset_t sa_mask;
	unsigned long sa_flags;
	void (*sa_restorer)(void);
};

#define linux_sa_handler	_u._sa_handler
#define linux_sa_sigaction	_u._sa_sigaction

#elif defined(__x86_64__)

// Modified to include sa_sigaction (I don't know why
// sa_sigaction isn't included by default...)
struct linux_sigaction {
	union {
		__linux_sighandler_t _sa_handler;
		linux_sa_sigaction_t _sa_sigaction;
	} _u;
	unsigned long sa_flags;
	__linux_sigrestore_t sa_restorer;
	linux_sigset_t sa_mask;		/* mask last for extensibility */
};

#define linux_sa_handler	_u._sa_handler
#define linux_sa_sigaction	_u._sa_sigaction

#else
#error "Missing sigaction struct for arch"
#endif

typedef struct linux_sigaltstack {
	void *ss_sp;
	int ss_flags;
	linux___kernel_size_t ss_size;
} linux_stack_t;

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_SIGNAL_H
