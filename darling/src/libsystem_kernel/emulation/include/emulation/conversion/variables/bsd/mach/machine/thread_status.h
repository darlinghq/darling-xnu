#ifndef DARLING_EMULATION_CONVERSION_BSD_MACH_MACHINE_THREAD_STATUS_H
#define DARLING_EMULATION_CONVERSION_BSD_MACH_MACHINE_THREAD_STATUS_H

#if defined (__i386__) || defined(__x86_64__)
#include <emulation/conversion/variables/bsd/mach/i386/thread_status.h>
#else 
#error "Missing 'thread_status.h' for arch"
#endif 

#endif // DARLING_EMULATION_CONVERSION_BSD_MACH_MACHINE_THREAD_STATUS_H