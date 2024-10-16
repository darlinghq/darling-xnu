#ifndef DARLING_EMULATION_CONVERSION_SIGACTION_H
#define DARLING_EMULATION_CONVERSION_SIGACTION_H

#include <emulation/conversion/signal.h>
#include <emulation/xnu_syscall/bsd/impl/signal/sigaltstack.h>

#undef sa_sigaction

#define BSD_SA_ONSTACK      0x0001
#define BSD_SA_RESTART      0x0002
#define BSD_SA_RESETHAND    0x0004
#define BSD_SA_NOCLDSTOP    0x0008
#define BSD_SA_NODEFER      0x0010
#define BSD_SA_NOCLDWAIT    0x0020
#define BSD_SA_SIGINFO      0x0040

#define LINUX_SA_NOCLDSTOP    0x00000001u
#define LINUX_SA_NOCLDWAIT    0x00000002u
#define LINUX_SA_SIGINFO      0x00000004u
#define LINUX_SA_ONSTACK      0x08000000u
#define LINUX_SA_RESTART      0x10000000u
#define LINUX_SA_NODEFER      0x40000000u
#define LINUX_SA_RESETHAND    0x80000000u

# define __SI_MAX_SIZE     128
# if defined (__x86_64__)
#  define __SI_PAD_SIZE     ((__SI_MAX_SIZE / sizeof (int)) - 4)
# else
#  define __SI_PAD_SIZE     ((__SI_MAX_SIZE / sizeof (int)) - 3)
# endif

struct linux_siginfo
{
	int si_signo;
	int si_errno;
	int si_code;
	union
	{
		struct
		{
			int si_pid;
			int si_uid;
		};
		void* si_addr;
	};

	union
	{
		int _pad[__SI_PAD_SIZE];
		unsigned long si_value;
	};
};

typedef void (linux_sig_handler)(int, struct linux_siginfo*, void*);

struct linux_sigaction
{
	linux_sig_handler* sa_sigaction;
	int sa_flags;
	void (*sa_restorer)(void);
	linux_sigset_t sa_mask;
};

void handler_linux_to_bsd(int linux_signum, struct linux_siginfo* info, void* ctxt);

extern int sig_flags[32];
extern unsigned int sig_masks[32];

extern int sigflags_bsd_to_linux(int flags);
extern int sigflags_linux_to_bsd(int flags);

#endif // DARLING_EMULATION_CONVERSION_SIGACTION_H
