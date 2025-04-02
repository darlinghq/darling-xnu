#ifndef DARLING_MACH_SYSCALL_TABLE_H
#define DARLING_MACH_SYSCALL_TABLE_H

#include <emulation/xnu_syscall/mach/impl/mach_other.h>
#include <emulation/xnu_syscall/mach/impl/mach_time.h>
#include <emulation/xnu_syscall/mach/impl/macx.h>
#include <emulation/xnu_syscall/mach/impl/mk_timer.h>
#include <emulation/xnu_syscall/mach/impl/trap/kernelrpc.h>
#include <emulation/xnu_syscall/mach/impl/trap/semaphore.h>

extern void* __mach_syscall_table[128];

#endif // DARLING_MACH_SYSCALL_TABLE_H
