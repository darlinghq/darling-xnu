#ifndef DARLING_EMULATION_CONVERSION_CONTEXT_H
#define DARLING_EMULATION_CONVERSION_CONTEXT_H

#include <_libkernel_init.h>

#include <emulation/conversion/signal.h>
#include <emulation/xnu_syscall/bsd/impl/signal/sigaltstack.h>

//
// Linux Context
//

#ifdef __x86_64__
typedef struct _fpstate {
        unsigned short cwd, swd, ftw, fop;
        unsigned long long rip, rdp;
        unsigned mxcsr, mxcr_mask;
        struct {
                unsigned short significand[4], exponent, padding[3];
        } _st[8];
        struct {
                unsigned element[4];
        } _xmm[16];
        unsigned padding[24];
} *linux_fpregset_t;

struct linux_gregset
{
	long long r8, r9, r10, r11, r12, r13, r14, r15, rdi, rsi, rbp, rbx;
	long long rdx, rax, rcx, rsp, rip, efl;
	short cs, gs, fs, __pad0;
	long long err, trapno, oldmask, cr2;
};

#elif defined(__i386__) // now the i386 version
typedef struct _fpstate {
        unsigned long cw, sw, tag, ipoff, cssel, dataoff, datasel;
        struct {
                unsigned short significand[4], exponent;
        } _st[8];
        unsigned short status, magic;
		unsigned int _fxsr_env[6];
		unsigned int mxcsr;
		unsigned int reserved;
		struct _fpxreg {
			unsigned short significand[4];
			unsigned short exponent;
			unsigned short padding[3];
		} _fxsr_st[8];
        struct {
                unsigned element[4];
        } _xmm[8];
} *linux_fpregset_t;

struct linux_gregset
{
	int gs, fs, es, ds, edi, esi, ebp, esp, ebx, edx, ecx, eax;
	int trapno, err, eip, cs, efl, uesp;
	int ss;
};

#else
#error "Missing Register Set for Arch"
#endif

struct linux_mcontext
{
	struct linux_gregset gregs;
	linux_fpregset_t fpregs;
#ifdef __x86_64__
	unsigned long long __reserved[8];
#else
	unsigned long oldmask, cr2;
#endif
	// +reserved
};

struct linux_ucontext
{
	unsigned long uc_flags;
	struct linux_ucontext* uc_link;
	struct linux_stack uc_stack;
	struct linux_mcontext uc_mcontext;
	linux_sigset_t uc_sigmask;
	// linux_libc_fpstate fpregs_mem;
};

//
// BSD States
//

struct bsd_exception_state
{
	unsigned short trapno;
	unsigned short cpu;
	unsigned int err;
	unsigned long faultvaddr;
};

struct bsd_thread_state
{
#ifdef __x86_64__
	long long rax, rbx, rcx, rdx, rdi, rsi, rbp, rsp, r8, r9, r10;
	long long r11, r12, r13, r14, r15, rip, rflags, cs, fs, gs;
#elif defined(__i386__)
	int eax, ebx, ecx, edx, edi, esi, ebp, esp, ss, eflags;
	int eip, cs, ds, es, fs, gs;
#else
#error "Missing BSD Thread State for Arch"
#endif
};

struct bsd_float_state
{
	// TODO
};

struct bsd_mcontext
{
	struct bsd_exception_state es;
	struct bsd_thread_state ss;
	struct bsd_float_state fs;
};

struct bsd_ucontext
{
	int uc_onstack;
	sigset_t uc_sigmask;
	struct bsd_stack uc_stack;
	struct bsd_ucontext* uc_link;
	unsigned long uc_mcsize;
	struct bsd_mcontext* uc_mcontext;
};

void ucontext_linux_to_bsd(const struct linux_ucontext* lc, struct bsd_ucontext* bc, struct bsd_mcontext* bm);
void mcontext_bsd_to_linux(const struct bsd_mcontext* bm, struct linux_mcontext* lm);

#if defined(__x86_64__)
void mcontext_to_thread_state(const struct linux_gregset* regs, x86_thread_state64_t* s);
void mcontext_to_float_state(const linux_fpregset_t fx, x86_float_state64_t* s);
void thread_state_to_mcontext(const x86_thread_state64_t* s, struct linux_gregset* regs);
void float_state_to_mcontext(const x86_float_state64_t* s, linux_fpregset_t fx);
#elif defined(__i386__)
void mcontext_to_thread_state(const struct linux_gregset* regs, x86_thread_state32_t* s);
void mcontext_to_float_state(const linux_fpregset_t fx, x86_float_state32_t* s);
void thread_state_to_mcontext(const x86_thread_state32_t* s, struct linux_gregset* regs);
void float_state_to_mcontext(const x86_float_state32_t* s, linux_fpregset_t fx);
#endif

#endif // DARLING_EMULATION_CONVERSION_CONTEXT_H