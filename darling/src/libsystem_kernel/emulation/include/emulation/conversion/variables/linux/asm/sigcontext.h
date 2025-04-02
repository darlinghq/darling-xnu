#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_SIGCONTEXT_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_SIGCONTEXT_H

#include <emulation/conversion/variables/linux/linux/types.h>

struct _linux_fpx_sw_bytes {
	linux___u32				magic1;
	linux___u32				extended_size;
	linux___u64				xfeatures;
	linux___u32				xstate_size;
	linux___u32				padding[7];
};

struct _linux_fpreg {
	linux___u16				significand[4];
	linux___u16				exponent;
};

struct _linux_fpxreg {
	linux___u16				significand[4];
	linux___u16				exponent;
	linux___u16				padding[3];
};

struct _linux_xmmreg {
	linux___u32				element[4];
};

#if defined(__i386__)
struct _linux_fpstate {
	/* Legacy FPU environment: */
	linux___u32				cw;
	linux___u32				sw;
	linux___u32				tag;
	linux___u32				ipoff;
	linux___u32				cssel;
	linux___u32				dataoff;
	linux___u32				datasel;
	struct _linux_fpreg			_st[8];
	linux___u16				status;
	linux___u16				magic;		/* 0xffff: regular FPU data only */
							/* 0x0000: FXSR FPU data */

	/* FXSR FPU environment */
	linux___u32				_fxsr_env[6];	/* FXSR FPU env is ignored */
	linux___u32				mxcsr;
	linux___u32				reserved;
	struct _linux_fpxreg			_fxsr_st[8];	/* FXSR FPU reg data is ignored */
	struct _linux_xmmreg			_xmm[8];	/* First 8 XMM registers */
	union {
		linux___u32			padding1[44];	/* Second 8 XMM registers plus padding */
		linux___u32			padding[44];	/* Alias name for old user-space */
	};

	union {
		linux___u32			padding2[12];
		struct _linux_fpx_sw_bytes	sw_reserved;	/* Potential extended state is encoded here */
	};
};

struct linux_sigcontext {
	linux___u16				gs, __gsh;
	linux___u16				fs, __fsh;
	linux___u16				es, __esh;
	linux___u16				ds, __dsh;
	linux___u32				edi;
	linux___u32				esi;
	linux___u32				ebp;
	linux___u32				esp;
	linux___u32				ebx;
	linux___u32				edx;
	linux___u32				ecx;
	linux___u32				eax;
	linux___u32				trapno;
	linux___u32				err;
	linux___u32				eip;
	linux___u16				cs, __csh;
	linux___u32				eflags;
	linux___u32				esp_at_signal;
	linux___u16				ss, __ssh;
	struct _linux_fpstate 	*fpstate;
	linux___u32				oldmask;
	linux___u32				cr2;
};

#elif defined(__x86_64__)
struct _linux_fpstate {
	linux___u16				cwd;
	linux___u16				swd;
	/* Note this is not the same as the 32-bit/x87/FSAVE twd: */
	linux___u16				twd;
	linux___u16				fop;
	linux___u64				rip;
	linux___u64				rdp;
	linux___u32				mxcsr;
	linux___u32				mxcsr_mask;
	linux___u32				st_space[32];	/*  8x  FP registers, 16 bytes each */
	linux___u32				xmm_space[64];	/* 16x XMM registers, 16 bytes each */
	linux___u32				reserved2[12];
	union {
		linux___u32			reserved3[12];
		struct _linux_fpx_sw_bytes	sw_reserved;	/* Potential extended state is encoded here */
	};
};

struct linux_sigcontext {
	linux___u64				r8;
	linux___u64				r9;
	linux___u64				r10;
	linux___u64				r11;
	linux___u64				r12;
	linux___u64				r13;
	linux___u64				r14;
	linux___u64				r15;
	linux___u64				rdi;
	linux___u64				rsi;
	linux___u64				rbp;
	linux___u64				rbx;
	linux___u64				rdx;
	linux___u64				rax;
	linux___u64				rcx;
	linux___u64				rsp;
	linux___u64				rip;
	linux___u64				eflags;		/* RFLAGS */
	linux___u16				cs;

	/*
	 * Prior to 2.5.64 ("[PATCH] x86-64 updates for 2.5.64-bk3"),
	 * Linux saved and restored fs and gs in these slots.  This
	 * was counterproductive, as fsbase and gsbase were never
	 * saved, so arch_prctl was presumably unreliable.
	 *
	 * These slots should never be reused without extreme caution:
	 *
	 *  - Some DOSEMU versions stash fs and gs in these slots manually,
	 *    thus overwriting anything the kernel expects to be preserved
	 *    in these slots.
	 *
	 *  - If these slots are ever needed for any other purpose,
	 *    there is some risk that very old 64-bit binaries could get
	 *    confused.  I doubt that many such binaries still work,
	 *    though, since the same patch in 2.5.64 also removed the
	 *    64-bit set_thread_area syscall, so it appears that there
	 *    is no TLS API beyond modify_ldt that works in both pre-
	 *    and post-2.5.64 kernels.
	 *
	 * If the kernel ever adds explicit fs, gs, fsbase, and gsbase
	 * save/restore, it will most likely need to be opt-in and use
	 * different context slots.
	 */
	linux___u16				gs;
	linux___u16				fs;
	union {
		linux___u16			ss;	/* If UC_SIGCONTEXT_SS */
		linux___u16			__pad0;	/* Alias name for old (!UC_SIGCONTEXT_SS) user-space */
	};
	linux___u64				err;
	linux___u64				trapno;
	linux___u64				oldmask;
	linux___u64				cr2;
	struct _linux_fpstate 	*fpstate;	/* Zero when no FPU context */
#  ifdef __ILP32__
	linux___u32				__fpstate_pad;
#  endif
	linux___u64				reserved1[8];
};

#else
#error "Missing sigcontext for arch"
#endif

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_SIGCONTEXT_H
