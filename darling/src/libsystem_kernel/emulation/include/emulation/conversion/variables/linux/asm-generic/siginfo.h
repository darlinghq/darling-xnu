#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_SIGINFO_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_SIGINFO_H

#include <emulation/conversion/variables/linux/linux/types.h>

typedef union linux_sigval {
	int sival_int;
	void *sival_ptr;
} linux_sigval_t;

#define LINUX_SI_MAX_SIZE	128

union __linux_sifields
{
	/* kill() */
	struct {
		linux___kernel_pid_t _pid;	/* sender's pid */
		linux___kernel_uid32_t _uid;	/* sender's uid */
	} _kill;

	/* POSIX.1b signals */
	struct {
		linux___kernel_pid_t _pid;	/* sender's pid */
		linux___kernel_uid32_t _uid;	/* sender's uid */
		linux_sigval_t _sigval;
	} _rt;

	/* SIGILL, SIGFPE, SIGSEGV, SIGBUS, SIGTRAP, SIGEMT */
	struct {
		void *_addr; /* faulting insn/memory ref. */
        // union {}
	} _sigfault;
};

#if defined(__x86_64__) || defined(__i386__)
struct linux_siginfo
{
	int si_signo;
	int si_errno;
	int si_code;

    union {
        union __linux_sifields _sifields;
        int _si_pad[LINUX_SI_MAX_SIZE/sizeof(int)];
    };
};
#else
#error "Verify siginfo order for arch"
#endif

#define linux_si_pid		_sifields._kill._pid
#define linux_si_uid		_sifields._kill._uid
#define linux_si_value      _sifields._rt._sigval
#define linux_si_int		linux_si_value.sival_int
#define linux_si_ptr		linux_si_value.sival_ptr
#define linux_si_addr		_sifields._sigfault._addr

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_SIGINFO_H
