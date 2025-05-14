#include <darling/emulation/xnu_syscall/bsd/impl/audit/audit_session_self.h>

mach_port_name_t sys_audit_session_self(void)
{
	//printf("audit_session_self STUB\n");
	return MACH_PORT_NULL;
}
