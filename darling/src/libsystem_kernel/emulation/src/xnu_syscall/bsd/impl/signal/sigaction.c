#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigaction.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/signal/sigexc.h>

#include <darlingserver/rpc.h>

static int sigflags_bsd_to_linux(int flags);
static int sigflags_linux_to_bsd(int flags);
extern void sig_restorer(void);

extern void* memcpy(void* dest, const void* src, __SIZE_TYPE__ len);
extern void* memset(void* dest, int v, __SIZE_TYPE__ len);

static void handler_linux_to_bsd_wrapper(int linux_signum, struct linux_siginfo* info, void* ctxt);

// Libc uses only one trampoline
void (*sa_tramp)(void*, int, int, struct bsd_siginfo*, void*) = 0;
bsd_sig_handler* sig_handlers[32] = {
	[LINUX_SIGWINCH] = (bsd_sig_handler*) SIG_IGN,
	[LINUX_SIGURG] = (bsd_sig_handler*) SIG_IGN,
};
int sig_flags[32];
unsigned int sig_masks[32];

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
		if (nsa->sa_sigaction != XNU_SIG_DFL && nsa->sa_sigaction != XNU_SIG_IGN
				&& nsa->sa_sigaction != XNU_SIG_ERR)
		{
			sa.sa_sigaction = &handler_linux_to_bsd_wrapper;
		}
		else
			sa.sa_sigaction = (linux_sig_handler*) nsa->sa_sigaction;

		sigset_bsd_to_linux(&nsa->sa_mask, &sa.sa_mask);
		sa.sa_flags = sigflags_bsd_to_linux(nsa->sa_flags) | LINUX_SA_RESTORER;
		sa.sa_restorer = sig_restorer;

		ret = LINUX_SYSCALL(__NR_rt_sigaction, linux_signum,
			(nsa != NULL) ? &sa : NULL, &olsa,
			sizeof(sa.sa_mask));

		if (ret < 0)
			return errno_linux_to_bsd(ret);

		if (osa != NULL)
		{
			if (olsa.sa_sigaction == handler_linux_to_bsd_wrapper)
				osa->sa_sigaction = sig_handlers[linux_signum];
			else // values such as SIG_DFL
				osa->sa_sigaction = (bsd_sig_handler*) olsa.sa_sigaction;
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
		sa.sa_restorer = sig_restorer;
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
};

static int sigflags_bsd_to_linux(int flags)
{
	int linux_flags = LINUX_SA_SIGINFO;

	if (flags & BSD_SA_NOCLDSTOP)
		linux_flags |= LINUX_SA_NOCLDSTOP;
	if (flags & BSD_SA_NOCLDWAIT)
		linux_flags |= LINUX_SA_NOCLDWAIT;
	if (flags & BSD_SA_ONSTACK)
		linux_flags |= LINUX_SA_ONSTACK;
	if (flags & BSD_SA_NODEFER)
		linux_flags |= LINUX_SA_NODEFER;
	if (flags & BSD_SA_RESETHAND)
		linux_flags |= LINUX_SA_RESETHAND;
	if (flags & BSD_SA_RESTART)
		linux_flags |= LINUX_SA_RESTART;

	return linux_flags;
}

static int sigflags_linux_to_bsd(int flags)
{
	int bsd_flags = 0;

	if (flags & LINUX_SA_NOCLDSTOP)
		bsd_flags |= BSD_SA_NOCLDSTOP;
	if (flags & LINUX_SA_NOCLDWAIT)
		bsd_flags |= BSD_SA_NOCLDWAIT;
	if (flags & LINUX_SA_ONSTACK)
		bsd_flags |= BSD_SA_ONSTACK;
	if (flags & LINUX_SA_NODEFER)
		bsd_flags |= BSD_SA_NODEFER;
	if (flags & LINUX_SA_RESETHAND)
		bsd_flags |= BSD_SA_RESETHAND;
	if (flags & LINUX_SA_RESTART)
		bsd_flags |= BSD_SA_RESTART;

	return bsd_flags;
}
