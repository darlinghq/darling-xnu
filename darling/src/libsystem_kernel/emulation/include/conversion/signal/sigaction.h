#ifndef DARLING_CONVERSION_SIGNAL_SIGACTION
#define DARLING_CONVERSION_SIGNAL_SIGACTION

#include <darling/emulation/conversion/signal/duct_signals.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/sigaltstack.h>

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

typedef void (bsd_sig_handler)(int, struct bsd_siginfo*, void*);
typedef void (linux_sig_handler)(int, struct linux_siginfo*, void*);

#ifndef XNU_SIG_DFL
#define XNU_SIG_DFL (bsd_sig_handler*)0
#endif
#ifndef XNU_SIG_IGN
#define XNU_SIG_IGN (bsd_sig_handler*)1
#endif
#ifndef XNU_SIG_ERR
#define XNU_SIG_ERR ((bsd_sig_handler*)-1l)
#endif

typedef void (*bsd_sig_tramp)(void*, int, int, struct bsd_siginfo*, void*);

struct bsd_sigaction
{
	bsd_sig_handler* sa_sigaction;
	unsigned int sa_mask;
	int sa_flags;
};

struct bsd___sigaction
{
	bsd_sig_handler* sa_sigaction;
	bsd_sig_tramp sa_tramp;
	unsigned int sa_mask;
	int sa_flags;
};

struct linux_sigaction
{
	linux_sig_handler* sa_sigaction;
	int sa_flags;
	void (*sa_restorer)(void);
	linux_sigset_t sa_mask;
};

#if defined(__x86_64__)
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

#elif defined(__i386__)

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

#elif defined(__aarch64__) || defined(__arm64__)

// ARM64 Linux FPSIMD context header
struct linux_aarch64_ctx {
	unsigned int magic;
	unsigned int size;
};

// ARM64 Linux FPSIMD state (embedded in sigcontext.__reserved)
struct linux_fpsimd_context {
	struct linux_aarch64_ctx head;
	unsigned int fpsr;
	unsigned int fpcr;
	__uint128_t vregs[32];
};

typedef struct linux_fpsimd_context *linux_fpregset_t;

// ARM64 Linux general-purpose registers (matches sigcontext layout)
struct linux_gregset
{
	unsigned long long regs[31]; // x0-x30
	unsigned long long sp;
	unsigned long long pc;
	unsigned long long pstate;
	unsigned long long fault_address;
};

#endif

struct linux_mcontext
{
#if defined(__aarch64__) || defined(__arm64__)
	struct linux_gregset gregs;
	// On ARM64 Linux, fpsimd is in __reserved area of sigcontext
	unsigned char __reserved[4096] __attribute__((__aligned__(16)));
#else
	struct linux_gregset gregs;
	linux_fpregset_t fpregs;
#ifdef __x86_64__
	unsigned long long __reserved[8];
#else
	unsigned long oldmask, cr2;
#endif
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

#if defined(__aarch64__) || defined(__arm64__)
struct bsd_exception_state
{
	unsigned long long faultvaddr; // __far (virtual fault address at offset 0)
	unsigned int err;              // __esr (exception syndrome)
	unsigned int trapno;           // __exception
};

struct bsd_thread_state
{
	unsigned long long x[29];
	unsigned long long fp, lr, sp, pc;
	unsigned int cpsr;
	unsigned int _pad;
};

struct bsd_float_state
{
	__uint128_t v[32];
	unsigned int fpsr;
	unsigned int fpcr;
	unsigned int _pad;
};
#elif defined(__x86_64__)
struct bsd_exception_state
{
	unsigned short trapno;
	unsigned short cpu;
	unsigned int err;
	unsigned long faultvaddr;
};

struct bsd_thread_state
{
	long long rax, rbx, rcx, rdx, rdi, rsi, rbp, rsp, r8, r9, r10;
	long long r11, r12, r13, r14, r15, rip, rflags, cs, fs, gs;
};

struct bsd_float_state
{
	int fpu_reserved[2];
	short fpu_fcw;
	short fpu_fsw;
	unsigned char fpu_ftw;
	unsigned char fpu_rsrv1;
	unsigned short fpu_fop;
	unsigned int fpu_ip;
	unsigned short fpu_cs;
	unsigned short fpu_rsrv2;
	unsigned int fpu_dp;
	unsigned short fpu_ds;
	unsigned short fpu_rsrv3;
	unsigned int fpu_mxcsr;
	unsigned int fpu_mxcsrmask;
	unsigned char fpu_stmm[128];
	unsigned char fpu_xmm[256];
	unsigned char fpu_rsrv4[96];
	int fpu_reserved1;
};
#elif defined(__i386__)
struct bsd_exception_state
{
	unsigned short trapno;
	unsigned short cpu;
	unsigned int err;
	unsigned long faultvaddr;
};

struct bsd_thread_state
{
	int eax, ebx, ecx, edx, edi, esi, ebp, esp, ss, eflags;
	int eip, cs, ds, es, fs, gs;
};

struct bsd_float_state
{
	unsigned char fpu_bytes[512];
};
#endif

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

void handler_linux_to_bsd(int linux_signum, struct linux_siginfo* info, void* ctxt);

#endif // DARLING_CONVERSION_SIGNAL_SIGACTION
