#ifndef DARLING_EMULATION_CONVERSION_BSD_MACHINE__MCONTEXT_H
#define DARLING_EMULATION_CONVERSION_BSD_MACHINE__MCONTEXT_H

#if defined(__i386__) || defined(__x86_64__)
#include <emulation/conversion/variables/bsd/i386/_mcontext.h>
#else
#error "Missing '_mcontext.h' for arch"
#endif

#endif // DARLING_EMULATION_CONVERSION_BSD_MACHINE__MCONTEXT_H
