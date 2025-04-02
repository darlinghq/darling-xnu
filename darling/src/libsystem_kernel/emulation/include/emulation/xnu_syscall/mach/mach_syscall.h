#ifndef DARLING_MACH_SYSCALL_H
#define DARLING_MACH_SYSCALL_H

#include <emulation/common/simple.h>

#define UNIMPLEMENTED_TRAP() __simple_printf("Called unimplemented Mach trap: %s\n", __FUNCTION__);

long __darling_mach_unknown_sysnum(int nr, ...);

#endif // DARLING_MACH_SYSCALL_H
