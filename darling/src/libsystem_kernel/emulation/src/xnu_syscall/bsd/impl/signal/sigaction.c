#include <emulation/xnu_syscall/bsd/impl/signal/sigaction.h>

#include <stddef.h>
#include <darlingserver/rpc.h>

#include <emulation/conversion/methods/signal/sigaction.h>
#include <emulation/linux_api/linux_syscall.h>

extern void* memset(void* dest, int v, __SIZE_TYPE__ len);

extern void linux_sig_restorer(void);

static void handler_linux_to_bsd_wrapper(int linux_signum, struct linux_siginfo* info, void* ctxt);

bsd_sig_handler* sig_handlers[32] = {
	[LINUX_SIGWINCH] = (bsd_sig_handler*) BSD_SIG_IGN,
	[LINUX_SIGURG] = (bsd_sig_handler*) BSD_SIG_IGN,
};

// Libc uses only one trampoline
void (*sa_tramp)(void*, int, int, struct bsd_siginfo*, void*) = 0;

long sys_sigaction(int signum, const struct bsd___sigaction* nsa, struct bsd_sigaction* osa)
{
	int ret, linux_signum;
	struct linux_sigaction sa, olsa;

	linux_signum = signum_bsd_to_linux(signum);
	if (linux_signum == 0)
	{
		// Some software (e.g. Node.js) tries to set up handlers for all signals by
		// walking through all signal numbers incrementally. They end up hitting
		// signals that don't exist on Linux and then bail out if they receive
		// an error.
		// Fake that everyting is all right.

		if (osa != NULL)
			memset(osa, 0, sizeof(*osa));
		
		return 0;
	}

	if (nsa != NULL && (linux_signum == LINUX_SIGCHLD || linux_signum == LINUX_SIGTTOU))
	{
		sa_tramp = nsa->sa_tramp;
		if (nsa->sa_sigaction != BSD_SIG_DFL && nsa->sa_sigaction != BSD_SIG_IGN
				&& nsa->sa_sigaction != BSD_SIG_ERR)
		{
			sa.linux_sa_sigaction = &handler_linux_to_bsd_wrapper;
		}
		else
			sa.linux_sa_sigaction = (linux_sa_sigaction_t) nsa->sa_sigaction;

		sigset_bsd_to_linux(&nsa->sa_mask, &sa.sa_mask);
		sa.sa_flags = sigflags_bsd_to_linux(nsa->sa_flags) | LINUX_SA_RESTORER;
		sa.sa_restorer = linux_sig_restorer;

		ret = LINUX_SYSCALL(__NR_rt_sigaction, linux_signum,
			(nsa != NULL) ? &sa : NULL, &olsa,
			sizeof(sa.sa_mask));

		if (ret < 0)
			return errno_linux_to_bsd(ret);

		if (osa != NULL)
		{
			if (olsa.linux_sa_sigaction == handler_linux_to_bsd_wrapper)
				osa->sa_sigaction = sig_handlers[linux_signum];
			else // values such as BSD_SIG_DFL
				osa->sa_sigaction = (bsd_sig_handler*) olsa.linux_sa_sigaction;
			sigset_linux_to_bsd(&olsa.sa_mask, &osa->sa_mask);
			osa->sa_flags = sigflags_linux_to_bsd(olsa.sa_flags);
		}
	}
	else
	{
		ret = 0;

		if (osa != NULL)
		{
			osa->sa_sigaction = sig_handlers[linux_signum];
			osa->sa_flags = sig_flags[linux_signum];
			osa->sa_mask = sig_masks[linux_signum];
		}
	}
	
	if (nsa != NULL && ret >= 0)
	{
		// __simple_printf("Saving handler for signal %d: %p\n", linux_signum, nsa->sa_sigaction);
		sig_handlers[linux_signum] = nsa->sa_sigaction;
		sig_flags[linux_signum] = nsa->sa_flags;
		sig_masks[linux_signum] = nsa->sa_mask;
	}

	return 0;
}

#if 0
long sys_sigaction(int signum, const struct bsd___sigaction* nsa, struct bsd_sigaction* osa)
{
	int ret, linux_signum;
	struct linux_sigaction sa, olsa;

	linux_signum = signum_bsd_to_linux(signum);
	if (linux_signum == 0)
	{
		// Some software (e.g. Node.js) tries to set up handlers for all signals by
		// walking through all signal numbers incrementally. They end up hitting
		// signals that don't exist on Linux and then bail out if they receive
		// an error.
		// Fake that everyting is all right.

		if (osa != NULL)
			memset(osa, 0, sizeof(*osa));
		
		return 0;
	}

	if (nsa != NULL)
	{
		sa_tramp = nsa->sa_tramp;
		/*if (darling_am_i_ptraced())
		{
			sa.sa_sigaction = &sigexc_handler;
		}
		else*/ if (nsa->sa_sigaction != SIG_DFL && nsa->sa_sigaction != SIG_IGN
				&& nsa->sa_sigaction != SIG_ERR)
		{
			sa.sa_sigaction = &handler_linux_to_bsd;
		}
		else
			sa.sa_sigaction = (linux_sig_handler*) nsa->sa_sigaction;

		sigset_bsd_to_linux(&nsa->sa_mask, &sa.sa_mask);
		sa.sa_flags = sigflags_bsd_to_linux(nsa->sa_flags) | LINUX_SA_RESTORER;
		sa.sa_restorer = linux_sig_restorer;
	}

	ret = LINUX_SYSCALL(__NR_rt_sigaction, linux_signum,
			(nsa != NULL) ? &sa : NULL, &olsa,
			sizeof(sa.sa_mask));
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	if (osa != NULL)
	{
		if (olsa.sa_sigaction == handler_linux_to_bsd)
			osa->sa_sigaction = sig_handlers[linux_signum];
		else // values such as SIG_DFL
			osa->sa_sigaction = (bsd_sig_handler*) olsa.sa_sigaction;
		sigset_linux_to_bsd(&olsa.sa_mask, &osa->sa_mask);
		osa->sa_flags = sigflags_linux_to_bsd(olsa.sa_flags);
	}
	
	if (nsa != NULL && ret >= 0)
	{
		//  __simple_printf("Saving handler for signal %d: %d\n", linux_signum, nsa->sa_sigaction);
		sig_handlers[linux_signum] = nsa->sa_sigaction;
		sig_flags[linux_signum] = sa.sa_flags;
		sig_masks[linux_signum] = sa.sa_mask;
	}

	return 0;
}
#endif

static void handler_linux_to_bsd_wrapper(int linux_signum, struct linux_siginfo* info, void* ctxt) {
	dserver_rpc_interrupt_enter();
	handler_linux_to_bsd(linux_signum, info, ctxt);
	dserver_rpc_interrupt_exit();
}
