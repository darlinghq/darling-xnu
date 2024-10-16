#include <emulation/common/simple.h>
#include <emulation/conversion/signal/sigexc.h>

#define DEBUG_SIGEXC
#ifdef DEBUG_SIGEXC
#define kern_printf(...) __simple_kprintf(__VA_ARGS__)
#else
#define kern_printf(...)
#endif

#define DUMPREG(regname) kern_printf("sigexc:   " #regname ": 0x%llx\n", regs->regname);

// TODO: The code here is copied from both sigaction.c and sigexc.c, one thing that bothers me is that
// there is a lot of unnecessary code duplication...

void ucontext_linux_to_bsd(const struct linux_ucontext* lc, struct bsd_ucontext* bc, struct bsd_mcontext* bm)
{
	bc->uc_onstack = 1;
	bc->uc_mcsize = sizeof(struct bsd_mcontext);
	bc->uc_mcontext = bm;
	
	sigset_linux_to_bsd(&lc->uc_sigmask, &bc->uc_sigmask);
	
	bc->uc_stack.ss_flags = lc->uc_stack.ss_flags;
	bc->uc_stack.ss_size = lc->uc_stack.ss_size;
	bc->uc_stack.ss_sp = lc->uc_stack.ss_sp;
	
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

#elif defined(__i386__)
	copyreg(eax); copyreg(ebx); copyreg(ecx); copyreg(edx); copyreg(edi); copyreg(esi);
	copyreg(ebp); copyreg(esp);
	copyreg(eip); copyreg(cs); copyreg(ds); copyreg(es); copyreg(fs); copyreg(gs);
	bm->ss.eflags = lc->uc_mcontext.gregs.efl;
	bm->ss.ss = 0;

#else
    #warning Missing code for current arch
#endif
	
    #undef copyreg
}

void mcontext_bsd_to_linux(const struct bsd_mcontext* bm, struct linux_mcontext* lm)
{
    #define copyreg(__name) lm->gregs.__name = bm->ss.__name

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

#else
    #warning Missing code for current arch
#endif

    #undef copyreg
}

#if defined(__x86_64__)
void mcontext_to_thread_state(const struct linux_gregset* regs, x86_thread_state64_t* s)
{
	s->__rax = regs->rax;
	s->__rbx = regs->rbx;
	s->__rcx = regs->rcx;
	s->__rdx = regs->rdx;
	s->__rdi = regs->rdi;
	s->__rsi = regs->rsi;
	s->__rbp = regs->rbp;
	s->__rsp = regs->rsp;
	s->__r8 = regs->r8;
	s->__r9 = regs->r9;
	s->__r10 = regs->r10;
	s->__r11 = regs->r11;
	s->__r12 = regs->r12;
	s->__r13 = regs->r13;
	s->__r14 = regs->r14;
	s->__r15 = regs->r15;
	s->__rip = regs->rip;
	s->__rflags = regs->efl;
	s->__cs = regs->cs;
	s->__fs = regs->fs;
	s->__gs = regs->gs;

	kern_printf("sigexc: saving to kernel:\n");
	DUMPREG(rip);
	DUMPREG(efl);
	DUMPREG(rax);
	DUMPREG(rbx);
	DUMPREG(rcx);
	DUMPREG(rdx);
	DUMPREG(rdi);
	DUMPREG(rsi);
	DUMPREG(rbp);
	DUMPREG(rsp);
}

void mcontext_to_float_state(const linux_fpregset_t fx, x86_float_state64_t* s)
{
	// fpregs is NULL on WSL1. See https://github.com/microsoft/WSL/issues/2555.
	if (fx == NULL)
	{
		memset(s, 0, sizeof(x86_float_state64_t));
		return;
	}

	kern_printf("sigexc: fcw out: 0x%x", fx->cwd);
	kern_printf("sigexc: xmm0 out: 0x%x", fx->_xmm[0].element[0]);
	*((uint16_t*)&s->__fpu_fcw) = fx->cwd;
	*((uint16_t*)&s->__fpu_fsw) = fx->swd;
	s->__fpu_ftw = fx->ftw;
	s->__fpu_fop = fx->fop;
	s->__fpu_ip = fx->rip;
	s->__fpu_cs = 0;
	s->__fpu_dp = fx->rdp;
	s->__fpu_ds = 0;
	s->__fpu_mxcsr = fx->mxcsr;
	s->__fpu_mxcsrmask = fx->mxcr_mask;

	memcpy(&s->__fpu_stmm0, fx->_st, 128);
	memcpy(&s->__fpu_xmm0, fx->_xmm, 256);
}

void thread_state_to_mcontext(const x86_thread_state64_t* s, struct linux_gregset* regs)
{
	regs->rax = s->__rax;
	regs->rbx = s->__rbx;
	regs->rcx = s->__rcx;
	regs->rdx = s->__rdx;
	regs->rdi = s->__rdi;
	regs->rsi = s->__rsi;
	regs->rbp = s->__rbp;
	regs->rsp = s->__rsp;
	regs->r8 = s->__r8;
	regs->r9 = s->__r9;
	regs->r10 = s->__r10;
	regs->r11 = s->__r11;
	regs->r12 = s->__r12;
	regs->r13 = s->__r13;
	regs->r14 = s->__r14;
	regs->r15 = s->__r15;
	regs->rip = s->__rip;
	regs->efl = s->__rflags;
	regs->cs = s->__cs;
	regs->fs = s->__fs;
	regs->gs = s->__gs;

	kern_printf("sigexc: restored from kernel:\n");
	DUMPREG(rip);
	DUMPREG(efl);
	DUMPREG(rax);
	DUMPREG(rbx);
	DUMPREG(rcx);
	DUMPREG(rdx);
	DUMPREG(rdi);
	DUMPREG(rsi);
	DUMPREG(rbp);
	DUMPREG(rsp);
}

void float_state_to_mcontext(const x86_float_state64_t* s, linux_fpregset_t fx)
{
	if (fx == NULL)
	{
		return;
	}

	fx->cwd = *((uint16_t*)&s->__fpu_fcw);
	fx->swd = *((uint16_t*)&s->__fpu_fsw);
	fx->ftw = s->__fpu_ftw;
	fx->fop = s->__fpu_fop;
	fx->rip = s->__fpu_ip;
	fx->rdp = s->__fpu_dp;
	fx->mxcsr = s->__fpu_mxcsr;
	fx->mxcr_mask = s->__fpu_mxcsrmask;

	memcpy(fx->_st, &s->__fpu_stmm0, 128);
	memcpy(fx->_xmm, &s->__fpu_xmm0, 256);
	kern_printf("sigexc: fcw in: 0x%x", fx->cwd);
	kern_printf("sigexc: xmm0 in: 0x%x", fx->_xmm[0].element[0]);
}

#elif defined(__i386__)
void mcontext_to_thread_state(const struct linux_gregset* regs, x86_thread_state32_t* s)
{
	s->__eax = regs->eax;
	s->__ebx = regs->ebx;
	s->__ecx = regs->ecx;
	s->__edx = regs->edx;
	s->__edi = regs->edi;
	s->__esi = regs->esi;
	s->__ebp = regs->ebp;
	s->__esp = regs->esp;
	s->__ss = regs->ss;
	s->__eflags = regs->efl;
	s->__eip = regs->eip;
	s->__cs = regs->cs;
	s->__ds = regs->ds;
	s->__es = regs->es;
	s->__fs = regs->fs;
	s->__gs = regs->gs;
}

void mcontext_to_float_state(const linux_fpregset_t fx, x86_float_state32_t* s)
{
	*((uint16_t*)&s->__fpu_fcw) = fx->cw;
	*((uint16_t*)&s->__fpu_fsw) = fx->sw;
	s->__fpu_ftw = fx->tag;
	s->__fpu_fop = 0;
	s->__fpu_ip = fx->ipoff;
	s->__fpu_cs = fx->cssel;
	s->__fpu_dp = fx->dataoff;
	s->__fpu_ds = fx->datasel;
	s->__fpu_mxcsr = fx->mxcsr;
	s->__fpu_mxcsrmask = 0;

	memcpy(&s->__fpu_stmm0, fx->_st, 128);
	memcpy(&s->__fpu_xmm0, fx->_xmm, 128);
	memset(((char*) &s->__fpu_xmm0) + 128, 0, 128);
}

void thread_state_to_mcontext(const x86_thread_state32_t* s, struct linux_gregset* regs)
{
	regs->eax = s->__eax;
	regs->ebx = s->__ebx;
	regs->ecx = s->__ecx;
	regs->edx = s->__edx;
	regs->edi = s->__edi;
	regs->esi = s->__esi;
	regs->ebp = s->__ebp;
	regs->esp = s->__esp;
	regs->ss = s->__ss;
	regs->efl = s->__eflags;
	regs->eip = s->__eip;
	regs->cs = s->__cs;
	regs->ds = s->__ds;
	regs->es = s->__es;
	regs->fs = s->__fs;
	regs->gs = s->__gs;
}

void float_state_to_mcontext(const x86_float_state32_t* s, linux_fpregset_t fx)
{
	fx->cw = *((uint16_t*)&s->__fpu_fcw);
	fx->sw = *((uint16_t*)&s->__fpu_fsw);
	fx->tag = s->__fpu_ftw;
	fx->ipoff = s->__fpu_ip;
	fx->cssel = s->__fpu_cs;
	fx->dataoff = s->__fpu_dp;
	fx->datasel = s->__fpu_ds;
	fx->mxcsr = s->__fpu_mxcsr;

	memcpy(fx->_st, &s->__fpu_stmm0, 128);
	memcpy(fx->_xmm, &s->__fpu_xmm0, 128);
}
#endif