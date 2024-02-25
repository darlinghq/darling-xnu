#include "sigaction.h"
#include "../base.h"
#include "../errno.h"
#include "../simple.h"
#include <linux-syscalls/linux.h>
#include <stddef.h>
#include "sigexc.h"
#include <sys/errno.h>

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
		if (nsa->sa_sigaction != SIG_DFL && nsa->sa_sigaction != SIG_IGN
				&& nsa->sa_sigaction != SIG_ERR)
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

#ifdef __arm64__
// Some values are stored in the __reserved array.
void grab_contexts_from_reserved(struct linux_mcontext* lm, struct fpsimd_context** reserved_neon_context, struct esr_context** reserved_esr_el1_context) {
	struct _aarch64_ctx* reserved_header = (struct _aarch64_ctx*)lm->__reserved;
	const struct _aarch64_ctx* RESERVED_MAX_LIMIT = (struct _aarch64_ctx*)lm->__reserved + 4096;
	do {
		if (reserved_header + reserved_header->size > RESERVED_MAX_LIMIT) {
			break;
		}

		if (reserved_header->magic == FPSIMD_MAGIC) {
			*reserved_neon_context = (struct fpsimd_context*)reserved_header;
		} else if (reserved_header->magic == ESR_MAGIC) {
			*reserved_esr_el1_context = (struct esr_context*)reserved_header;
		}

		reserved_header += reserved_header->size;

	// The last structure must be a dummy one with the magic and size set to 0
	} while (reserved_header->magic != 0 && reserved_header->size != 0);
}
#endif

static void ucontext_linux_to_bsd(const struct linux_ucontext* lc, struct bsd_ucontext* bc, struct bsd_mcontext* bm)
{
	bc->uc_onstack = 1;
	bc->uc_mcsize = sizeof(struct bsd_mcontext);
	bc->uc_mcontext = bm;
	
	sigset_linux_to_bsd(&lc->uc_sigmask, &bc->uc_sigmask);
	
	bc->uc_stack.ss_flags = lc->uc_stack.ss_flags;
	bc->uc_stack.ss_size = lc->uc_stack.ss_size;
	bc->uc_stack.ss_sp = lc->uc_stack.ss_sp;

	
#ifdef __arm64__
	struct fpsimd_context* reserved_neon_context = NULL;
	struct esr_context* reserved_esr_el1_context = NULL;
	grab_contexts_from_reserved(&lc->uc_mcontext, &reserved_neon_context, &reserved_esr_el1_context);
#endif

#if defined(__x86_64__) || defined(__i386__)
	bm->es.trapno = lc->uc_mcontext.gregs.trapno;
	bm->es.cpu = 0;
	bm->es.err = lc->uc_mcontext.gregs.err;
#ifdef __x86_64__
	bm->es.faultvaddr = lc->uc_mcontext.gregs.rip;
#elif defined(__i386__)
	bm->es.faultvaddr = lc->uc_mcontext.gregs.eip;
#endif
#elif defined(__arm64__)
	bm->es.far = lc->uc_mcontext.fault_address;
	bm->es.esr = 0;
	bm->es.exception = 0;
	if (reserved_esr_el1_context != NULL) {
		// I not really sure how I should handle this...
		// esr on Linux is 64bits, while esr on macOS is 32bits
		bm->es.esr = 0xFFFFFFFF & reserved_esr_el1_context->esr;
		bm->es.exception = 0xFFFFFFFF & (reserved_esr_el1_context->esr >> 32);
	}
#else
#error "Missing exception state (Linux to BSD) conversion"
#endif

#if defined(__x86_64__) || defined(__i386___)
#define copyreg(__name) bm->ss.__name = lc->uc_mcontext.gregs.__name
#elif defined(__arm64__)
#define copyreg2(__bsdname, __linuxname) bm->ss.__bsdname = lc->uc_mcontext.__linuxname
#define copyreg(__name) copyreg2(__name, __name)
#endif

#ifdef __x86_64__
	copyreg(rax); copyreg(rbx); copyreg(rcx); copyreg(rdx); copyreg(rdi); copyreg(rsi);
	copyreg(rbp); copyreg(rsp); copyreg(r8); copyreg(r9); copyreg(r10);
	copyreg(r11); copyreg(r12); copyreg(r13); copyreg(r14); copyreg(r15); copyreg(rip);
	copyreg(cs); copyreg(fs); copyreg(gs);
	bm->ss.rflags = lc->uc_mcontext.gregs.efl;

#elif defined(__i386__)
	copyreg(eax); copyreg(ebx); copyreg(ecx); copyreg(edx); copyreg(edi); copyreg(esi);
	copyreg(ebp); copyreg(esp);
	copyreg(eip); copyreg(cs); copyreg(ds); copyreg(es); copyreg(fs); copyreg(gs);
	bm->ss.eflags = lc->uc_mcontext.gregs.efl;
	bm->ss.ss = 0;

#elif defined(__arm64__)
	for (int i=0; i<29; i++) {
		copyreg2(x[i],regs[i]);
	}
	copyreg2(fp,regs[29]); copyreg2(lr,regs[30]);
	copyreg(sp); copyreg(pc);

	bm->ss.cpsr = 0xFFFFFFFF & lc->uc_mcontext.pstate;
	bm->ss.flags = 0xFFFFFFFF & (lc->uc_mcontext.pstate >> 32);

#else
#error Missing thread state (Linux to BSD) conversion
#endif

#undef copyreg
#undef copyreg2

#if defined(__x86_64__) || defined(__i386__)
#warning "Missing float state (Linux to BSD) conversion for x86_64/i386"

#elif defined(__arm64__)
#define copyreg2(__bsdname, __linuxname) bm->ns.__bsdname = reserved_neon_context->__linuxname
#define copyreg(__name) copyreg2(__name, __name)

	if (reserved_neon_context != NULL) {
		for (int i=0; i<32; i++) {
			copyreg2(q[i], vregs[i]);
		}
		copyreg(fpsr);
		copyreg(fpcr);
	}

#else
#error "Missing float/simd state (Linux to BSD) conversion"
#endif

#undef copyreg
#undef copyreg2

}

static void mcontext_bsd_to_linux(const struct bsd_mcontext* bm, struct linux_mcontext* lm)
{
#if defined(__x86_64__) || defined(__i386__)
#define copyreg(__name) lm->gregs.__name = bm->ss.__name
#elif defined(__arm64__)
#define copyreg2(__linuxname, __bsdname) lm->__linuxname = bm->ss.__bsdname;
#define copyreg(__name) copyreg2(__name, __name)
#endif

#ifdef __arm64__
	struct fpsimd_context* reserved_neon_context = NULL;
	struct esr_context* reserved_esr_el1_context = NULL;
	grab_contexts_from_reserved(lm, &reserved_neon_context, &reserved_esr_el1_context);
#endif

#ifdef __x86_64__
	copyreg(rax); copyreg(rbx); copyreg(rcx); copyreg(rdx); copyreg(rdi); copyreg(rsi);
	copyreg(rbp); copyreg(rsp); copyreg(r8); copyreg(r9); copyreg(r10);
	copyreg(r11); copyreg(r12); copyreg(r13); copyreg(r14); copyreg(r15); copyreg(rip);
	copyreg(cs); copyreg(fs); copyreg(gs);
	lm->gregs.efl = bm->ss.rflags;
#elif defined(__i386__)
	copyreg(eax); copyreg(ebx); copyreg(ecx); copyreg(edx); copyreg(edi); copyreg(esi);
	copyreg(ebp); copyreg(esp);
	copyreg(eip); copyreg(cs); copyreg(ds); copyreg(es); copyreg(fs); copyreg(gs);
	lm->gregs.efl = bm->ss.eflags;
#elif defined(__arm64__)
	for (int i=0; i<29; i++) {
		copyreg2(regs[i],x[i]);
	}
	copyreg2(regs[29],fp); copyreg2(regs[30],lr);
	copyreg(sp); copyreg(pc);

	lm->pstate = bm->ss.cpsr;
	lm->pstate |= (long long int)bm->ss.flags << 32;

	if (reserved_esr_el1_context != NULL) {
		reserved_esr_el1_context->esr = bm->es.esr;
		reserved_esr_el1_context->esr |= (__uint64_t)reserved_esr_el1_context->esr << 32;
	}
#else
#error Missing thread state (BSD to Linux) conversion
#endif

#undef copyreg
#undef copyreg2

#if defined(__arm64__)
#define copyreg2(__linuxname, __bsdname) reserved_neon_context->__linuxname = bm->ns.__bsdname;
#define copyreg(__name) copyreg2(__name, __name)
#endif

#ifdef __arm64__
	if (reserved_neon_context != NULL) {
		for (int i=0; i<32; i++) {
			copyreg2(vregs[i],q[i]);
		}
		copyreg(fpsr);
		copyreg(fpcr);
	}
#endif

#undef copyreg
#undef copyreg2

}

static void handler_linux_to_bsd_wrapper(int linux_signum, struct linux_siginfo* info, void* ctxt) {
	dserver_rpc_interrupt_enter();
	handler_linux_to_bsd(linux_signum, info, ctxt);
	dserver_rpc_interrupt_exit();
};

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
				sig_handlers[linux_signum] = (bsd_sig_handler*) SIG_IGN;
				break;
			default:
				sig_handlers[linux_signum] = NULL; // SIG_DFL
		}
	}

	handler(bsd_signum, info ? &binfo : NULL, (lc != NULL) ? &bc : NULL);

	if (lc != NULL)
	{
		mcontext_bsd_to_linux(bc.uc_mcontext, &lc->uc_mcontext);
	}
	
	// __simple_printf("Signal handled\n");
}

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

