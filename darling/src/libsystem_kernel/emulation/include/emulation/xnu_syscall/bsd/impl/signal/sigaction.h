#ifndef DARLING_BSD_EMULATION_SIGACTION_H
#define DARLING_BSD_EMULATION_SIGACTION_H

#include <emulation/conversion/variables/bsd/sys/signal.h>

struct bsd_siginfo
{
	int si_signo;
	int si_errno;
	int si_code;
	unsigned int si_pid;
	unsigned int si_uid;
	int si_status;
	void* si_addr;
	void* si_val_ptr;
	long si_band;
	unsigned long __pad[7];
};

typedef void (bsd_sig_handler)(int, struct bsd_siginfo*, void*);

struct bsd_sigaction
{
	bsd_sig_handler* sa_sigaction;
	unsigned int sa_mask;
	int sa_flags;
};

struct bsd___sigaction
{
	bsd_sig_handler* sa_sigaction;
	void (*sa_tramp)(void*, int, int, struct bsd_siginfo*, void*);
	unsigned int sa_mask;
	int sa_flags;
};

long sys_sigaction(int signum, const struct bsd___sigaction* nsa, struct bsd_sigaction* osa);

extern bsd_sig_handler* sig_handlers[32];

#endif // DARLING_BSD_EMULATION_SIGACTION_H