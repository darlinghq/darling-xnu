#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__UCONTEXT64_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__UCONTEXT64_H

#include <emulation/conversion/variables/bsd/machine/_mcontext.h>
#include <emulation/conversion/variables/bsd/machine/types.h>
#include <emulation/conversion/variables/bsd/sys/_types.h>
#include <emulation/conversion/variables/bsd/sys/_types/_sigaltstack.h>

struct bsd_ucontext64
{
	int                     uc_onstack;
	bsd___darwin_sigset_t   uc_sigmask;
	struct bsd_sigaltstack  uc_stack;
	struct bsd_ucontext64   *uc_link;
	bsd___darwin_size_t     uc_mcsize;
	struct bsd_mcontext64   *uc_mcontext64;
};

typedef struct bsd_ucontext64      bsd_ucontext64_t;

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__UCONTEXT64_H