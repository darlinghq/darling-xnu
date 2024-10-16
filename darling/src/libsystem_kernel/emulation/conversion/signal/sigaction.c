#include "sigaction.h"

#include <stddef.h>
#include <sys/errno.h>

#include <emulation/common/simple.h>
#include <emulation/conversion/errno.h>
#include <emulation/conversion/signal/context.h>
#include <emulation/conversion/signal/sigexc.h>
#include <emulation/xnu_syscall/bsd/impl/signal/sigaction.h>

#undef memset
// extern void* memcpy(void* dest, const void* src, __SIZE_TYPE__ len);
extern void* memset(void* dest, int v, __SIZE_TYPE__ len);

int sig_flags[32];
unsigned int sig_masks[32];


void handler_linux_to_bsd(int linux_signum, struct linux_siginfo* info, void* ctxt)
{
	int bsd_signum;
	struct bsd_siginfo binfo;
	struct linux_ucontext* lc = (struct linux_ucontext*) ctxt;
	struct bsd_ucontext bc;

	bsd_signum = signum_linux_to_bsd(linux_signum);

	if (info)
	{
		memset(&binfo, 0, sizeof(binfo));
		binfo.si_signo = signum_linux_to_bsd(info->si_signo);
		binfo.si_errno = errno_linux_to_bsd(info->si_errno);
		binfo.si_code = info->si_code;
		binfo.si_pid = info->si_pid;
		binfo.si_uid = info->si_uid;
		binfo.si_addr = info->si_addr;
		binfo.si_val_ptr = (void*) info->si_value;
		
		// TODO: The following exist on Linux, but it's a mess to extract them
		binfo.si_status = 0;
		binfo.si_band = 0;
	}
	
	if (lc != NULL)
	{
		ucontext_linux_to_bsd(lc, &bc, (struct bsd_mcontext*) __builtin_alloca(sizeof(struct bsd_mcontext)));
		if (lc->uc_link != NULL)
		{
			struct bsd_ucontext* bc_link = (struct bsd_ucontext*) __builtin_alloca(sizeof(struct bsd_ucontext));
			
			ucontext_linux_to_bsd(lc->uc_link, bc_link, (struct bsd_mcontext*) __builtin_alloca(sizeof(struct bsd_mcontext)));
			
			bc.uc_link = bc_link;
			bc_link->uc_link = NULL;
		}
	}
		
	// __simple_printf("Handling signal %d\n", linux_signum);

	bsd_sig_handler* handler = sig_handlers[linux_signum];
	if (sig_flags[linux_signum] & LINUX_SA_RESETHAND)
	{
		switch (linux_signum)
		{
			case LINUX_SIGWINCH:
			case LINUX_SIGCHLD:
			case LINUX_SIGURG:
				sig_handlers[linux_signum] = (bsd_sig_handler*)BSD_SIG_IGN;
				break;
			default:
				sig_handlers[linux_signum] = NULL; // BSD_SIG_DFL
		}
	}

	handler(bsd_signum, info ? &binfo : NULL, (lc != NULL) ? &bc : NULL);

	if (lc != NULL)
	{
		mcontext_bsd_to_linux(bc.uc_mcontext, &lc->uc_mcontext);
	}
	
	// __simple_printf("Signal handled\n");
}

int sigflags_bsd_to_linux(int flags)
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

int sigflags_linux_to_bsd(int flags)
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

