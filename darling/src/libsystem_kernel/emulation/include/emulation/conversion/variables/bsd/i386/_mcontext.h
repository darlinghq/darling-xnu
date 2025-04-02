#ifndef DARLING_EMULATION_CONVERSION_BSD_I386__MCONTEXT_H
#define DARLING_EMULATION_CONVERSION_BSD_I386__MCONTEXT_H

#include <emulation/conversion/variables/bsd/mach/machine/_structs.h>

struct bsd_mcontext32
{
	struct bsd_i386_exception_state   es;
	struct bsd_i386_thread_state      ss;
	struct bsd_i386_float_state       fs;
};

struct bsd_mcontext64
{
	struct bsd_x86_exception_state64   es;
	struct bsd_x86_thread_state64      ss;
	struct bsd_x86_exception_state64   fs;
};

#if defined(__x86_64__)
typedef struct bsd_mcontext64* bsd_mcontext_t;
#define bsd_mcontext bsd_mcontext64
#elif defined(__i386__)
typedef struct bsd_mcontext32* bsd_mcontext_t;
#define bsd_mcontext bsd_mcontext32
#endif

#endif // DARLING_EMULATION_CONVERSION_BSD_I386__MCONTEXT_H
