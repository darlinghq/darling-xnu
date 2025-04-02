#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__SIGALTSTACK_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__SIGALTSTACK_H

#include <darling/emulation/conversion/variables/bsd/machine/types.h>

struct bsd_sigaltstack
{
	void                *ss_sp;
	bsd___darwin_size_t ss_size;
	int                 ss_flags;
};
typedef struct bsd_sigaltstack     bsd_stack_t;

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__SIGALTSTACK_H
