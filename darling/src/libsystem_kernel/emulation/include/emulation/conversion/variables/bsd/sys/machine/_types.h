#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_MACHINE__TYPES_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_MACHINE__TYPES_H

#if defined(__x86_64__) || defined(__i386__)
#include <emulation/conversion/variables/bsd/i386/_types.h>
#else
#error "Missing types for arch"
#endif

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_MACHINE__TYPES_H
