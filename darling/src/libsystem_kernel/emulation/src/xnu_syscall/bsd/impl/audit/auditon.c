#include <darling/emulation/xnu_syscall/bsd/impl/audit/auditon.h>
#include <darling/emulation/xnu_syscall/bsd/impl/audit/audit_addr.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>

#include <stdint.h>
#include <bsm/audit.h>

extern void* memcpy(void* dest, const void* src, __SIZE_TYPE__ n);

/**
 * auditon(2) control of the BSM audit session.
 *
 * Darling has no audit event pipeline: it never writes audit records, and the
 * session it reports through getaudit_addr(2) is present but inert. This call
 * therefore maintains the session state that is genuinely observable - the
 * per-process audit mask and audit id - and validates its arguments the way
 * XNU's audit_syscalls.c does.
 *
 * It does not pretend to record anything. Commands that only make sense with a
 * live audit log - flushing, triggering rotation, pointing at an audit file,
 * changing the control mode - return ENOTSUP rather than a success that would
 * misrepresent a working audit trail.
 */

#define PID_MAX 99999

static int is_not_valid_pid(pid_t pid) {
	return pid < 1 || pid > PID_MAX;
}

long sys_auditon(int cmd, void* data, int length) {
	switch (cmd) {
		case A_SETPMASK: {
			// Set the audit mask of a process. XNU resolves the target
			// through proc_find(); only the calling process is reachable
			// from the emulation layer, so the mask is applied to the
			// session that this process reports through getaudit_addr.
			auditpinfo_t info;

			if (length != (int) sizeof(auditpinfo_t)) {
				return -EINVAL;
			}
			if (data == NULL) {
				return -EFAULT;
			}
			memcpy(&info, data, sizeof(auditpinfo_t));
			if (is_not_valid_pid(info.ap_pid)) {
				return -EINVAL;
			}
			audit_session_set_mask(info.ap_mask);
			return 0;
		}
		case A_SETKAUDIT: {
			// "Audit on fail" has nothing to fail over to without an
			// audit pipeline. The flag is accepted so that a caller
			// enabling it during login setup is not refused.
			if (length != (int) sizeof(int)) {
				return -EINVAL;
			}
			return 0;
		}
		case A_GETKAUDIT: {
			// Answerable: auditing is never active here, so the flag
			// always reads back as off.
			if (data == NULL || length < (int) sizeof(int)) {
				return -EINVAL;
			}
			*((int*) data) = 0;
			return 0;
		}
		case A_OLDSETPOLICY:
		case A_SETPOLICY: {
			// Validate the policy flags exactly as XNU does, then
			// accept: those flags only govern the audit log, which does
			// not exist here.
			int64_t policy;

			if (data == NULL) {
				return -EINVAL;
			}
			if (length == (int) sizeof(policy)) {
				memcpy(&policy, data, sizeof(policy));
				if (policy & ~(AUDIT_CNT | AUDIT_AHLT | AUDIT_ARGV |
							   AUDIT_ARGE)) {
					return -EINVAL;
				}
			}
			return 0;
		}
		case A_OLDGETCOND:
		case A_GETCOND: {
			// Report the audit condition. Darling never writes audit
			// records, so this is AUC_DISABLED unless a caller set it
			// through SETCOND. Returning it truthfully - rather than
			// failing - is what lets login(1) start; it treats an
			// unreadable condition as fatal.
			if (data == NULL || length < (int) sizeof(int)) {
				return -EINVAL;
			}
			*((int*) data) = audit_session_get_condition();
			return 0;
		}
		case A_OLDSETCOND:
		case A_SETCOND: {
			if (data == NULL || length < (int) sizeof(int)) {
				return -EINVAL;
			}
			audit_session_set_condition(*((int*) data));
			return 0;
		}
		default:
			return -ENOTSUP;
	}
}
