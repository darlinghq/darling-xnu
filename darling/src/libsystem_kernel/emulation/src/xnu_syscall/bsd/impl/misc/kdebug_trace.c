#include <darling/emulation/xnu_syscall/bsd/impl/misc/kdebug_trace.h>

long sys_kdebug_trace64(uint32_t code, uint64_t arg1, uint64_t arg2,
        uint64_t arg3, uint64_t arg4)
{
    (void)code;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    (void)arg4;

    // Darling does not enable kernel tracing. XNU's kdebug_trace64 returns
    // success before validating event IDs when kdebug_enable is zero.
    // This is the disabled-tracing path, not an event-recording backend.
    return 0;
}

long sys_kdebug_trace(uint32_t code, unsigned long arg1, unsigned long arg2,
        unsigned long arg3, unsigned long arg4)
{
    return sys_kdebug_trace64(code, arg1, arg2, arg3, arg4);
}
