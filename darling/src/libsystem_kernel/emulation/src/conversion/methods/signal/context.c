#include <emulation/conversion/methods/signal/context.h>

#include <emulation/common/simple.h>
#include <emulation/conversion/methods/signal/sigexc.h>

#include <stdint.h>

extern void *memset(void *s, int c, __SIZE_TYPE__ n);
extern void *memcpy(void *dest, const void *src, __SIZE_TYPE__ n);

#define DEBUG_SIGEXC
#ifdef DEBUG_SIGEXC
#define kern_printf(...) __simple_kprintf(__VA_ARGS__)
#else
#define kern_printf(...)
#endif

#define DUMPREG(regname) kern_printf("sigexc:   " #regname ": 0x%llx\n", sigcontext->regname);

// TODO: The code here is copied from both sigaction.c and sigexc.c, one thing that bothers me is that
// there is a lot of unnecessary code duplication...

void ucontext_linux_to_bsd(const struct linux_ucontext* lc, struct bsd_ucontext* bc, struct bsd_mcontext *bm)
{
	bc->uc_onstack = 1;
	bc->uc_mcsize = sizeof(struct bsd_mcontext);
	bc->uc_mcontext = bm;
	
	sigset_linux_to_bsd(&lc->uc_sigmask, &bc->uc_sigmask);
	
	bc->uc_stack.ss_flags = lc->uc_stack.ss_flags;
	bc->uc_stack.ss_size = lc->uc_stack.ss_size;
	bc->uc_stack.ss_sp = lc->uc_stack.ss_sp;
	
	bm->es.trapno = lc->uc_mcontext.trapno;
	bm->es.cpu = 0;
	bm->es.err = lc->uc_mcontext.err;
#ifdef __x86_64__
	bm->es.faultvaddr = lc->uc_mcontext.rip;
#else
	bm->es.faultvaddr = lc->uc_mcontext.eip;
#endif
	
    #define copyreg(__name) bm->ss.__name = lc->uc_mcontext.__name
	
#ifdef __x86_64__
	copyreg(rax); copyreg(rbx); copyreg(rcx); copyreg(rdx); copyreg(rdi); copyreg(rsi);
	copyreg(rbp); copyreg(rsp); copyreg(r8); copyreg(r9); copyreg(r10);
	copyreg(r11); copyreg(r12); copyreg(r13); copyreg(r14); copyreg(r15); copyreg(rip);
	copyreg(cs); copyreg(fs); copyreg(gs);
	bm->ss.rflags = lc->uc_mcontext.eflags;

#elif defined(__i386__)
	copyreg(eax); copyreg(ebx); copyreg(ecx); copyreg(edx); copyreg(edi); copyreg(esi);
	copyreg(ebp); copyreg(esp);
	copyreg(eip); copyreg(cs); copyreg(ds); copyreg(es); copyreg(fs); copyreg(gs);
	bm->ss.eflags = lc->uc_mcontext.eflags;
	bm->ss.ss = 0;

#else
    #warning Missing code for current arch
#endif
	
    #undef copyreg
}

void mcontext_bsd_to_linux(const struct bsd_mcontext *bm, struct linux_sigcontext* lm)
{
    #define copyreg(__name) lm->__name = bm->ss.__name

#ifdef __x86_64__
	copyreg(rax); copyreg(rbx); copyreg(rcx); copyreg(rdx); copyreg(rdi); copyreg(rsi);
	copyreg(rbp); copyreg(rsp); copyreg(r8); copyreg(r9); copyreg(r10);
	copyreg(r11); copyreg(r12); copyreg(r13); copyreg(r14); copyreg(r15); copyreg(rip);
	copyreg(cs); copyreg(fs); copyreg(gs);
	lm->eflags = bm->ss.rflags;

#elif defined(__i386__)
	copyreg(eax); copyreg(ebx); copyreg(ecx); copyreg(edx); copyreg(edi); copyreg(esi);
	copyreg(ebp); copyreg(esp);
	copyreg(eip); copyreg(cs); copyreg(ds); copyreg(es); copyreg(fs); copyreg(gs);
	lm->eflags = bm->ss.eflags;

#else
    #warning Missing code for current arch
#endif

    #undef copyreg
}

#if defined(__x86_64__)
void mcontext_to_thread_state(const struct linux_sigcontext* sigcontext, bsd_x86_thread_state64_t* s)
{
	s->rax = sigcontext->rax;
	s->rbx = sigcontext->rbx;
	s->rcx = sigcontext->rcx;
	s->rdx = sigcontext->rdx;
	s->rdi = sigcontext->rdi;
	s->rsi = sigcontext->rsi;
	s->rbp = sigcontext->rbp;
	s->rsp = sigcontext->rsp;
	s->r8 = sigcontext->r8;
	s->r9 = sigcontext->r9;
	s->r10 = sigcontext->r10;
	s->r11 = sigcontext->r11;
	s->r12 = sigcontext->r12;
	s->r13 = sigcontext->r13;
	s->r14 = sigcontext->r14;
	s->r15 = sigcontext->r15;
	s->rip = sigcontext->rip;
	s->rflags = sigcontext->eflags;
	s->cs = sigcontext->cs;
	s->fs = sigcontext->fs;
	s->gs = sigcontext->gs;

	kern_printf("sigexc: saving to kernel:\n");
	DUMPREG(rip);
	DUMPREG(eflags);
	DUMPREG(rax);
	DUMPREG(rbx);
	DUMPREG(rcx);
	DUMPREG(rdx);
	DUMPREG(rdi);
	DUMPREG(rsi);
	DUMPREG(rbp);
	DUMPREG(rsp);
}

void mcontext_to_float_state(const struct linux_sigcontext* sigcontext, bsd_x86_float_state64_t* s)
{
	// fpregs is NULL on WSL1. See https://github.com/microsoft/WSL/issues/2555.
	if (sigcontext->fpstate == NULL)
	{
		memset(s, 0, sizeof(bsd_x86_float_state64_t));
		return;
	}

	kern_printf("sigexc: fcw out: 0x%x", sigcontext->fpstate->cwd);
	kern_printf("sigexc: xmm0 out: 0x%x", sigcontext->fpstate->xmm_space[0]);
	*((uint16_t*)&s->fpu_fcw) = sigcontext->fpstate->cwd;
	*((uint16_t*)&s->fpu_fsw) = sigcontext->fpstate->swd;
	s->fpu_ftw = sigcontext->fpstate->twd;
	s->fpu_fop = sigcontext->fpstate->fop;
	s->fpu_ip = sigcontext->fpstate->rip;
	s->fpu_cs = 0;
	s->fpu_dp = sigcontext->fpstate->rdp;
	s->fpu_ds = 0;
	s->fpu_mxcsr = sigcontext->fpstate->mxcsr;
	s->fpu_mxcsrmask = sigcontext->fpstate->mxcsr_mask;

	memcpy(&s->fpu_stmm0, sigcontext->fpstate->st_space, 128);
	memcpy(&s->fpu_xmm0, sigcontext->fpstate->xmm_space, 256);
}

void thread_state_to_mcontext(const bsd_x86_thread_state64_t* s, struct linux_sigcontext* sigcontext)
{
	sigcontext->rax = s->rax;
	sigcontext->rbx = s->rbx;
	sigcontext->rcx = s->rcx;
	sigcontext->rdx = s->rdx;
	sigcontext->rdi = s->rdi;
	sigcontext->rsi = s->rsi;
	sigcontext->rbp = s->rbp;
	sigcontext->rsp = s->rsp;
	sigcontext->r8 = s->r8;
	sigcontext->r9 = s->r9;
	sigcontext->r10 = s->r10;
	sigcontext->r11 = s->r11;
	sigcontext->r12 = s->r12;
	sigcontext->r13 = s->r13;
	sigcontext->r14 = s->r14;
	sigcontext->r15 = s->r15;
	sigcontext->rip = s->rip;
	sigcontext->eflags = s->rflags;
	sigcontext->cs = s->cs;
	sigcontext->fs = s->fs;
	sigcontext->gs = s->gs;

	kern_printf("sigexc: restored from kernel:\n");
	DUMPREG(rip);
	DUMPREG(eflags);
	DUMPREG(rax);
	DUMPREG(rbx);
	DUMPREG(rcx);
	DUMPREG(rdx);
	DUMPREG(rdi);
	DUMPREG(rsi);
	DUMPREG(rbp);
	DUMPREG(rsp);
}

void float_state_to_mcontext(const bsd_x86_float_state64_t* s, struct linux_sigcontext* sigcontext)
{
	if (sigcontext->fpstate == NULL)
	{
		return;
	}

	sigcontext->fpstate->cwd = *((uint16_t*)&s->fpu_fcw);
	sigcontext->fpstate->swd = *((uint16_t*)&s->fpu_fsw);
	sigcontext->fpstate->twd = s->fpu_ftw;
	sigcontext->fpstate->fop = s->fpu_fop;
	sigcontext->fpstate->rip = s->fpu_ip;
	sigcontext->fpstate->rdp = s->fpu_dp;
	sigcontext->fpstate->mxcsr = s->fpu_mxcsr;
	sigcontext->fpstate->mxcsr_mask = s->fpu_mxcsrmask;

	memcpy(sigcontext->fpstate->st_space, &s->fpu_stmm0, 128);
	memcpy(sigcontext->fpstate->xmm_space, &s->fpu_xmm0, 256);
	kern_printf("sigexc: fcw in: 0x%x", sigcontext->fpstate->cwd);
	kern_printf("sigexc: xmm0 in: 0x%x", sigcontext->fpstate->xmm_space[0]);
}

#elif defined(__i386__)
void mcontext_to_thread_state(const struct linux_sigcontext* sigcontext, bsd_x86_thread_state32_t* s)
{
	s->eax = sigcontext->eax;
	s->ebx = sigcontext->ebx;
	s->ecx = sigcontext->ecx;
	s->edx = sigcontext->edx;
	s->edi = sigcontext->edi;
	s->esi = sigcontext->esi;
	s->ebp = sigcontext->ebp;
	s->esp = sigcontext->esp;
	s->ss = sigcontext->ss;
	s->eflags = sigcontext->eflags;
	s->eip = sigcontext->eip;
	s->cs = sigcontext->cs;
	s->ds = sigcontext->ds;
	s->es = sigcontext->es;
	s->fs = sigcontext->fs;
	s->gs = sigcontext->gs;
}

void mcontext_to_float_state(const struct linux_sigcontext* sigcontext, bsd_x86_float_state32_t* s)
{
	
	
	*((uint16_t*)&s->fpu_fcw) = sigcontext->fpstate->cw;
	*((uint16_t*)&s->fpu_fsw) = sigcontext->fpstate->sw;
	s->fpu_ftw = sigcontext->fpstate->tag;
	s->fpu_fop = 0;
	s->fpu_ip = sigcontext->fpstate->ipoff;
	s->fpu_cs = sigcontext->fpstate->cssel;
	s->fpu_dp = sigcontext->fpstate->dataoff;
	s->fpu_ds = sigcontext->fpstate->datasel;
	s->fpu_mxcsr = sigcontext->fpstate->mxcsr;
	s->fpu_mxcsrmask = 0;

	memcpy(&s->fpu_stmm0, sigcontext->fpstate->_st, 128);
	memcpy(&s->fpu_xmm0, sigcontext->fpstate->_xmm, 128);
	memset(((char*) &s->fpu_xmm0) + 128, 0, 128);
}

void thread_state_to_mcontext(const bsd_x86_thread_state32_t* s, struct linux_sigcontext* sigcontext)
{
	sigcontext->eax = s->eax;
	sigcontext->ebx = s->ebx;
	sigcontext->ecx = s->ecx;
	sigcontext->edx = s->edx;
	sigcontext->edi = s->edi;
	sigcontext->esi = s->esi;
	sigcontext->ebp = s->ebp;
	sigcontext->esp = s->esp;
	sigcontext->ss = s->ss;
	sigcontext->eflags = s->eflags;
	sigcontext->eip = s->eip;
	sigcontext->cs = s->cs;
	sigcontext->ds = s->ds;
	sigcontext->es = s->es;
	sigcontext->fs = s->fs;
	sigcontext->gs = s->gs;
}

void float_state_to_mcontext(const bsd_x86_float_state32_t* s, struct linux_sigcontext* sigcontext)
{
	sigcontext->fpstate->cw = *((uint16_t*)&s->fpu_fcw);
	sigcontext->fpstate->sw = *((uint16_t*)&s->fpu_fsw);
	sigcontext->fpstate->tag = s->fpu_ftw;
	sigcontext->fpstate->ipoff = s->fpu_ip;
	sigcontext->fpstate->cssel = s->fpu_cs;
	sigcontext->fpstate->dataoff = s->fpu_dp;
	sigcontext->fpstate->datasel = s->fpu_ds;
	sigcontext->fpstate->mxcsr = s->fpu_mxcsr;

	memcpy(sigcontext->fpstate->_st, &s->fpu_stmm0, 128);
	memcpy(sigcontext->fpstate->_xmm, &s->fpu_xmm0, 128);
}
#endif