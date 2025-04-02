#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_UCONTEXT_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_UCONTEXT_H

struct linux_ucontext {
	unsigned long	  uc_flags;
	struct linux_ucontext  *uc_link;
	linux_stack_t		  uc_stack;
	struct linux_sigcontext uc_mcontext;
	linux_sigset_t	  uc_sigmask;	/* mask last for extensibility */
};

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_UCONTEXT_H
