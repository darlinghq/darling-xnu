#ifndef _DARLING_EMULATION_AUDIT_AUDIT_SESSION_JOIN_H
#define _DARLING_EMULATION_AUDIT_AUDIT_SESSION_JOIN_H

#include <stdint.h>
#include <bsm/audit.h>

long sys_audit_session_join(mach_port_name_t audit_session);

#endif // _DARLING_EMULATION_AUDIT_AUDIT_SESSION_JOIN_H
