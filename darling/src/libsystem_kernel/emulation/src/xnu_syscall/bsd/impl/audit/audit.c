#include <darling/emulation/xnu_syscall/bsd/impl/audit/audit.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>

/**
 * audit(2) appends a record to the audit log.
 *
 * Darling has no audit log. This mirrors XNU's own behaviour for a kernel
 * built without audit support (CONFIG_AUDIT == 0), whose syscall stub in
 * bsd/security/audit/audit_syscalls.c returns ENOSYS as well. Callers treat
 * that as "not recorded" and carry on: login(1) prints "au_close() was not
 * committed" and continues to authenticate.
 *
 * Returning ENOSYS rather than 0 is deliberate. 0 would claim the record was
 * durably written, which would misrepresent an audit trail that does not exist.
 */
long sys_audit(void* record, int length) {
	(void) record;
	(void) length;
	return -ENOSYS;
}
