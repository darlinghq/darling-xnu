#include <darling/emulation/xnu_syscall/bsd/impl/misc/abort_with_payload.h>

#include <sys/signal.h>

#include <darling/emulation/xnu_syscall/bsd/impl/signal/kill.h>
#include <darling/emulation/common/simple.h>

long sys_abort_with_payload(unsigned int reason_namespace, unsigned long long reason_code, void *payload, unsigned int payload_size, const char *reason_string, unsigned long long reason_flags)
{
	__simple_printf("abort_with_payload: reason: %s; code: %lu\n", reason_string, reason_code);
	sys_kill(0, SIGABRT, 1);
	return 0;
}
