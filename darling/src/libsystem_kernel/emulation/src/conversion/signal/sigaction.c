#include <darling/emulation/conversion/signal/sigaction.h>

#include <stddef.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigaction.h>

extern void* memset(void* dest, int v, __SIZE_TYPE__ len);
extern void* memcpy(void* dest, const void* src, __SIZE_TYPE__ len);

void handler_linux_to_bsd(int linux_signum, struct linux_siginfo *info, void *ctxt);
static void mcontext_bsd_to_linux(const struct bsd_mcontext *bm, struct linux_mcontext *lm);
static void ucontext_linux_to_bsd(const struct linux_ucontext *lc, struct bsd_ucontext *bc, struct bsd_mcontext *bm);

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

static void ucontext_linux_to_bsd(const struct linux_ucontext* lc, struct bsd_ucontext* bc, struct bsd_mcontext* bm)
{
	bc->uc_onstack = 1;
	bc->uc_mcsize = sizeof(struct bsd_mcontext);
	bc->uc_mcontext = bm;
	
	sigset_linux_to_bsd(&lc->uc_sigmask, &bc->uc_sigmask);
	
	bc->uc_stack.ss_flags = lc->uc_stack.ss_flags;
	bc->uc_stack.ss_size = lc->uc_stack.ss_size;
	bc->uc_stack.ss_sp = lc->uc_stack.ss_sp;
	
#if defined(__aarch64__) || defined(__arm64__)
	bm->es.faultvaddr = lc->uc_mcontext.gregs.fault_address;
	bm->es.err = 0;
	bm->es.trapno = 0;

	for (int i = 0; i < 29; i++)
		bm->ss.x[i] = lc->uc_mcontext.gregs.regs[i];
	bm->ss.fp = lc->uc_mcontext.gregs.regs[29];
	bm->ss.lr = lc->uc_mcontext.gregs.regs[30];
	bm->ss.sp = lc->uc_mcontext.gregs.sp;
	bm->ss.pc = lc->uc_mcontext.gregs.pc;
	bm->ss.cpsr = (unsigned int)lc->uc_mcontext.gregs.pstate;
	bm->ss._pad = 0;

	memset(&bm->fs, 0, sizeof(bm->fs));
	const unsigned char* reserved = lc->uc_mcontext.__reserved;
	const unsigned char* end = reserved + sizeof(lc->uc_mcontext.__reserved);
	const struct linux_aarch64_ctx* ctx = (const struct linux_aarch64_ctx*)reserved;
	while ((const unsigned char*)ctx + sizeof(*ctx) <= end && ctx->magic != 0)
	{
		if (ctx->magic == 0x46508001 /* FPSIMD_MAGIC */)
		{
			const struct linux_fpsimd_context* fpsimd = (const struct linux_fpsimd_context*)ctx;
			if ((const unsigned char*)fpsimd + sizeof(*fpsimd) <= end)
			{
				bm->fs.fpsr = fpsimd->fpsr;
				bm->fs.fpcr = fpsimd->fpcr;
				memcpy(bm->fs.v, fpsimd->vregs, sizeof(bm->fs.v));
			}
			break;
		}
		if (ctx->size == 0)
			break;
		ctx = (const struct linux_aarch64_ctx*)((const unsigned char*)ctx + ctx->size);
	}
#else
	bm->es.trapno = lc->uc_mcontext.gregs.trapno;
	bm->es.cpu = 0;
	bm->es.err = lc->uc_mcontext.gregs.err;
#ifdef __x86_64__
	bm->es.faultvaddr = lc->uc_mcontext.gregs.rip;
#else
	bm->es.faultvaddr = lc->uc_mcontext.gregs.eip;
#endif

#define copyreg(__name) bm->ss.__name = lc->uc_mcontext.gregs.__name

#ifdef __x86_64__
	copyreg(rax); copyreg(rbx); copyreg(rcx); copyreg(rdx); copyreg(rdi); copyreg(rsi);
	copyreg(rbp); copyreg(rsp); copyreg(r8); copyreg(r9); copyreg(r10);
	copyreg(r11); copyreg(r12); copyreg(r13); copyreg(r14); copyreg(r15); copyreg(rip);
	copyreg(cs); copyreg(fs); copyreg(gs);
	bm->ss.rflags = lc->uc_mcontext.gregs.efl;

	if (lc->uc_mcontext.fpregs == NULL)
	{
		memset(&bm->fs, 0, sizeof(bm->fs));
	}
	else
	{
		const linux_fpregset_t fx = lc->uc_mcontext.fpregs;
		memset(&bm->fs, 0, sizeof(bm->fs));
		bm->fs.fpu_fcw = (short)fx->cwd;
		bm->fs.fpu_fsw = (short)fx->swd;
		bm->fs.fpu_ftw = (unsigned char)fx->ftw;
		bm->fs.fpu_fop = fx->fop;
		bm->fs.fpu_ip = (unsigned int)fx->rip;
		bm->fs.fpu_dp = (unsigned int)fx->rdp;
		bm->fs.fpu_mxcsr = fx->mxcsr;
		bm->fs.fpu_mxcsrmask = fx->mxcr_mask;
		memcpy(bm->fs.fpu_stmm, fx->_st, 128);
		memcpy(bm->fs.fpu_xmm, fx->_xmm, 256);
	}
#elif defined(__i386__)
	copyreg(eax); copyreg(ebx); copyreg(ecx); copyreg(edx); copyreg(edi); copyreg(esi);
	copyreg(ebp); copyreg(esp);
	copyreg(eip); copyreg(cs); copyreg(ds); copyreg(es); copyreg(fs); copyreg(gs);
	bm->ss.eflags = lc->uc_mcontext.gregs.efl;
	bm->ss.ss = 0;
	memset(&bm->fs, 0, sizeof(bm->fs));
#endif
#endif /* __aarch64__ || __arm64__ */

#undef copyreg
}

static void mcontext_bsd_to_linux(const struct bsd_mcontext* bm, struct linux_mcontext* lm)
{
#if defined(__aarch64__) || defined(__arm64__)
	for (int i = 0; i < 29; i++)
		lm->gregs.regs[i] = bm->ss.x[i];
	lm->gregs.regs[29] = bm->ss.fp;
	lm->gregs.regs[30] = bm->ss.lr;
	lm->gregs.sp = bm->ss.sp;
	lm->gregs.pc = bm->ss.pc;
	lm->gregs.pstate = bm->ss.cpsr;

	unsigned char* reserved = lm->__reserved;
	unsigned char* end = reserved + sizeof(lm->__reserved);
	struct linux_aarch64_ctx* ctx = (struct linux_aarch64_ctx*)reserved;
	while ((unsigned char*)ctx + sizeof(*ctx) <= end && ctx->magic != 0)
	{
		if (ctx->magic == 0x46508001 /* FPSIMD_MAGIC */)
		{
			struct linux_fpsimd_context* fpsimd = (struct linux_fpsimd_context*)ctx;
			if ((unsigned char*)fpsimd + sizeof(*fpsimd) <= end)
			{
				fpsimd->fpsr = bm->fs.fpsr;
				fpsimd->fpcr = bm->fs.fpcr;
				memcpy(fpsimd->vregs, bm->fs.v, sizeof(bm->fs.v));
			}
			break;
		}
		if (ctx->size == 0)
			break;
		ctx = (struct linux_aarch64_ctx*)((unsigned char*)ctx + ctx->size);
	}
#else
#define copyreg(__name) lm->gregs.__name = bm->ss.__name

#ifdef __x86_64__
	copyreg(rax); copyreg(rbx); copyreg(rcx); copyreg(rdx); copyreg(rdi); copyreg(rsi);
	copyreg(rbp); copyreg(rsp); copyreg(r8); copyreg(r9); copyreg(r10);
	copyreg(r11); copyreg(r12); copyreg(r13); copyreg(r14); copyreg(r15); copyreg(rip);
	copyreg(cs); copyreg(fs); copyreg(gs);
	lm->gregs.efl = bm->ss.rflags;

	if (lm->fpregs != NULL)
	{
		linux_fpregset_t fx = lm->fpregs;
		fx->cwd = (unsigned short)bm->fs.fpu_fcw;
		fx->swd = (unsigned short)bm->fs.fpu_fsw;
		fx->ftw = bm->fs.fpu_ftw;
		fx->fop = bm->fs.fpu_fop;
		fx->rip = bm->fs.fpu_ip;
		fx->rdp = bm->fs.fpu_dp;
		fx->mxcsr = bm->fs.fpu_mxcsr;
		fx->mxcr_mask = bm->fs.fpu_mxcsrmask;
		memcpy(fx->_st, bm->fs.fpu_stmm, 128);
		memcpy(fx->_xmm, bm->fs.fpu_xmm, 256);
	}
#elif defined(__i386__)
	copyreg(eax); copyreg(ebx); copyreg(ecx); copyreg(edx); copyreg(edi); copyreg(esi);
	copyreg(ebp); copyreg(esp);
	copyreg(eip); copyreg(cs); copyreg(ds); copyreg(es); copyreg(fs); copyreg(gs);
	lm->gregs.efl = bm->ss.eflags;
#endif
#endif /* __aarch64__ || __arm64__ */

#undef copyreg
}
