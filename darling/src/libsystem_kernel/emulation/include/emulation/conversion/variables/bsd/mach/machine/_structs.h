#ifndef DARLING_EMULATION_CONVERSION_BSD_MACH_MACHINE__STRUCTS_H
#define DARLING_EMULATION_CONVERSION_BSD_MACH_MACHINE__STRUCTS_H

#if defined(__i386__) || defined(__x86_64__)
#include <emulation/conversion/variables/bsd/mach/i386/_structs.h>
#else
#error "Missing _structs for arch"
#endif

#endif // DARLING_EMULATION_CONVERSION_BSD_MACH_MACHINE__STRUCTS_H