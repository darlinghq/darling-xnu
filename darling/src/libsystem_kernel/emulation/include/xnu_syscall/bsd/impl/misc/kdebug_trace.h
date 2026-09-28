#ifndef DARLING_EMULATION_KDEBUG_TRACE_H
#define DARLING_EMULATION_KDEBUG_TRACE_H

#include <stdint.h>

long sys_kdebug_trace(uint32_t code, unsigned long arg1, unsigned long arg2,
        unsigned long arg3, unsigned long arg4);
long sys_kdebug_trace64(uint32_t code, uint64_t arg1, uint64_t arg2,
        uint64_t arg3, uint64_t arg4);

#endif
