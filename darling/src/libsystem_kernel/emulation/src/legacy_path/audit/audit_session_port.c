#include <darling/emulation/legacy_path/audit/audit_session_port.h>

#include <darling/emulation/legacy_path/duct_errno.h>

long sys_audit_session_port(au_asid_t asid, mach_port_t* out_port_name) {
	return -EINVAL;
};
