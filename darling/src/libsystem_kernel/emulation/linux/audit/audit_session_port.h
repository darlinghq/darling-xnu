#ifndef _DARLING_EMULATION_AUDIT_AUDIT_SESSION_PORT_H
#define _DARLING_EMULATION_AUDIT_AUDIT_SESSION_PORT_H

#include <stdint.h>
#include <bsm/audit.h>

long sys_audit_session_port(au_asid_t asid, mach_port_t* out_port_name);

#endif // _DARLING_EMULATION_AUDIT_AUDIT_SESSION_PORT_H
