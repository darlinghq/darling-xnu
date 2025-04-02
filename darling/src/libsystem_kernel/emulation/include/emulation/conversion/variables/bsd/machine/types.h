#ifndef DARLING_EMULATION_CONVERSION_BSD_MACHINE_TYPES_H
#define DARLING_EMULATION_CONVERSION_BSD_MACHINE_TYPES_H

#if defined(__i386__) || defined(__x86_64__)
#include <darling/emulation/conversion/variables/bsd/i386/types.h>
#else
#error "Missing 'types.h' to arch"
#endif

#endif // DARLING_EMULATION_CONVERSION_BSD_MACHINE_TYPES_H
