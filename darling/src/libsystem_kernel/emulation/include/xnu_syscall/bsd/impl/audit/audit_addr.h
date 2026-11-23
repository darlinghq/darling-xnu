#ifndef _DARLING_EMULATION_AUDIT_AUDIT_ADDR_H
#define _DARLING_EMULATION_AUDIT_AUDIT_ADDR_H

#include <stdint.h>
#include <bsm/audit.h>

long sys_getaudit_addr(struct auditinfo_addr* auditinfo_addr, int length);
long sys_setaudit_addr(struct auditinfo_addr* auditinfo_addr, int length);

// Applies a change to the process audit session that -getaudit_addr reports, so
// that auditon(2) and the getaudit_addr/setaudit_addr calls agree.
void audit_session_set_mask(au_mask_t mask);
void audit_session_set_auid(au_id_t auid);
void audit_session_set_condition(int condition);

// Looks up an audit session by id for auditon(2)'s A_GETSINFO_ADDR. Only the
// session this process belongs to is known; any other id fails with -EINVAL,
// the error XNU gives for an id that names no session.
long audit_session_lookup(au_asid_t asid, struct auditinfo_addr* out);
int audit_session_get_condition(void);

#endif // _DARLING_EMULATION_AUDIT_GETAUDIT_ADDR_H
