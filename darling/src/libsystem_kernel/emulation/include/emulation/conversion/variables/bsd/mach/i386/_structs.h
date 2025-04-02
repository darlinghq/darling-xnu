#ifndef DARLING_EMULATION_CONVERSION_BSD_MACH_I386__STRUCTS_H
#define DARLING_EMULATION_CONVERSION_BSD_MACH_I386__STRUCTS_H

#include <emulation/conversion/variables/bsd/machine/types.h>

// #define	_STRUCT_X86_THREAD_STATE32	struct bsd_i386_thread_state
struct bsd_i386_thread_state
{
    unsigned int	eax;
    unsigned int	ebx;
    unsigned int	ecx;
    unsigned int	edx;
    unsigned int	edi;
    unsigned int	esi;
    unsigned int	ebp;
    unsigned int	esp;
    unsigned int	ss;
    unsigned int	eflags;
    unsigned int	eip;
    unsigned int	cs;
    unsigned int	ds;
    unsigned int	es;
    unsigned int	fs;
    unsigned int	gs;
};

// #define _STRUCT_FP_CONTROL	struct bsd_fp_control
struct bsd_fp_control
{
    unsigned short		invalid	:1,
    				denorm	:1,
				zdiv	:1,
				ovrfl	:1,
				undfl	:1,
				precis	:1,
					:2,
				pc	:2,
				rc	:2,
				/*inf*/	:1,
					:3;
};
typedef struct bsd_fp_control	bsd_fp_control_t;

// #define _STRUCT_FP_STATUS	struct bsd_fp_status
struct bsd_fp_status
{
    unsigned short		invalid	:1,
    				denorm	:1,
				zdiv	:1,
				ovrfl	:1,
				undfl	:1,
				precis	:1,
				stkflt	:1,
				errsumm	:1,
				c0	:1,
				c1	:1,
				c2	:1,
				tos	:3,
				c3	:1,
				busy	:1;
};
typedef struct bsd_fp_status	bsd_fp_status_t;

// #define _STRUCT_MMST_REG	struct bsd_mmst_reg
struct bsd_mmst_reg
{
	char	mmst_reg[10];
	char	mmst_rsrv[6];
};

// #define _STRUCT_XMM_REG		struct bsd_xmm_reg
struct bsd_xmm_reg
{
	char		xmm_reg[16];
};

// #define _STRUCT_X86_EXCEPTION_STATE32	struct bsd_i386_exception_state
struct bsd_i386_exception_state
{
	bsd___uint16_t	trapno;
	bsd___uint16_t	cpu;
	bsd___uint32_t	err;
	bsd___uint32_t	faultvaddr;
};

struct bsd_i386_float_state {
	int 			fpu_reserved[2];
	struct bsd_fp_control	fpu_fcw;
	struct bsd_fp_status	fpu_fsw;
	bsd___uint8_t			fpu_ftw;
	bsd___uint8_t			fpu_rsrv1;
	bsd___uint16_t			fpu_fop;
	bsd___uint32_t			fpu_ip;
	bsd___uint16_t			fpu_cs;
	bsd___uint16_t			fpu_rsrv2;
	bsd___uint32_t			fpu_dp;
	bsd___uint16_t			fpu_ds;
	bsd___uint16_t			fpu_rsrv3;
	bsd___uint32_t			fpu_mxcsr;
	bsd___uint32_t			fpu_mxcsrmask;
	struct bsd_mmst_reg		fpu_stmm0;
	struct bsd_mmst_reg		fpu_stmm1;
	struct bsd_mmst_reg		fpu_stmm2;
	struct bsd_mmst_reg		fpu_stmm3;
	struct bsd_mmst_reg		fpu_stmm4;
	struct bsd_mmst_reg		fpu_stmm5;
	struct bsd_mmst_reg		fpu_stmm6;
	struct bsd_mmst_reg		fpu_stmm7;
	struct bsd_xmm_reg		fpu_xmm0;
	struct bsd_xmm_reg		fpu_xmm1;
	struct bsd_xmm_reg		fpu_xmm2;
	struct bsd_xmm_reg		fpu_xmm3;
	struct bsd_xmm_reg		fpu_xmm4;
	struct bsd_xmm_reg		fpu_xmm5;
	struct bsd_xmm_reg		fpu_xmm6;
	struct bsd_xmm_reg		fpu_xmm7;
	char			fpu_rsrv4[14*16];	/* reserved */
	int 			fpu_reserved1;
};

struct bsd_x86_thread_state64
{
	bsd___uint64_t	rax;
	bsd___uint64_t	rbx;
	bsd___uint64_t	rcx;
	bsd___uint64_t	rdx;
	bsd___uint64_t	rdi;
	bsd___uint64_t	rsi;
	bsd___uint64_t	rbp;
	bsd___uint64_t	rsp;
	bsd___uint64_t	r8;
	bsd___uint64_t	r9;
	bsd___uint64_t	r10;
	bsd___uint64_t	r11;
	bsd___uint64_t	r12;
	bsd___uint64_t	r13;
	bsd___uint64_t	r14;
	bsd___uint64_t	r15;
	bsd___uint64_t	rip;
	bsd___uint64_t	rflags;
	bsd___uint64_t	cs;
	bsd___uint64_t	fs;
	bsd___uint64_t	gs;
};

struct bsd_x86_float_state64
{
	int 			fpu_reserved[2];
	struct bsd_fp_control	fpu_fcw;
	struct bsd_fp_status	fpu_fsw;
	bsd___uint8_t		fpu_ftw;
	bsd___uint8_t		fpu_rsrv1;
	bsd___uint16_t		fpu_fop;

	/* x87 FPU Instruction Pointer */
	bsd___uint32_t		fpu_ip;
	bsd___uint16_t		fpu_cs;

	bsd___uint16_t		fpu_rsrv2;

	/* x87 FPU Instruction Operand(Data) Pointer */
	bsd___uint32_t			fpu_dp;
	bsd___uint16_t			fpu_ds;

	bsd___uint16_t			fpu_rsrv3;
	bsd___uint32_t			fpu_mxcsr;
	bsd___uint32_t			fpu_mxcsrmask;
	struct bsd_mmst_reg		fpu_stmm0;
	struct bsd_mmst_reg		fpu_stmm1;
	struct bsd_mmst_reg		fpu_stmm2;
	struct bsd_mmst_reg		fpu_stmm3;
	struct bsd_mmst_reg		fpu_stmm4;
	struct bsd_mmst_reg		fpu_stmm5;
	struct bsd_mmst_reg		fpu_stmm6;
	struct bsd_mmst_reg		fpu_stmm7;
	struct bsd_xmm_reg		fpu_xmm0;
	struct bsd_xmm_reg		fpu_xmm1;
	struct bsd_xmm_reg		fpu_xmm2;
	struct bsd_xmm_reg		fpu_xmm3;
	struct bsd_xmm_reg		fpu_xmm4;
	struct bsd_xmm_reg		fpu_xmm5;
	struct bsd_xmm_reg		fpu_xmm6;
	struct bsd_xmm_reg		fpu_xmm7;
	struct bsd_xmm_reg		fpu_xmm8;
	struct bsd_xmm_reg		fpu_xmm9;
	struct bsd_xmm_reg		fpu_xmm10;
	struct bsd_xmm_reg		fpu_xmm11;
	struct bsd_xmm_reg		fpu_xmm12;
	struct bsd_xmm_reg		fpu_xmm13;
	struct bsd_xmm_reg		fpu_xmm14;
	struct bsd_xmm_reg		fpu_xmm15;
	char		fpu_rsrv4[6*16];
	int 		fpu_reserved1;
};

struct bsd_x86_exception_state64
{
    bsd___uint16_t	trapno;
    bsd___uint16_t	cpu;
    bsd___uint32_t	err;
    bsd___uint64_t	faultvaddr;
};

#endif // DARLING_EMULATION_CONVERSION_BSD_MACH_I386__STRUCTS_H