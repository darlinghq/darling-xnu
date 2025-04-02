#ifndef DARLING_EMULATION_CONVERSION_BSD_MACH_I386_THREAD_STATUS_H
#define DARLING_EMULATION_CONVERSION_BSD_MACH_I386_THREAD_STATUS_H

#include <emulation/conversion/variables/bsd/mach/i386/_structs.h> // Instead of <../mach/machine/_structs.h>

typedef struct bsd_i386_thread_state bsd_x86_thread_state32_t;

typedef struct bsd_i386_float_state bsd_x86_float_state32_t;

typedef struct bsd_x86_thread_state64 bsd_x86_thread_state64_t;

typedef struct bsd_x86_float_state64 bsd_x86_float_state64_t;

#endif // DARLING_EMULATION_CONVERSION_BSD_MACH_I386_THREAD_STATUS_H
