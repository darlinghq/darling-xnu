#include <darling/emulation/xnu_syscall/bsd/impl/audit/audit_addr.h>

#include <libsimple/lock.h>

#include <darling/emulation/xnu_syscall/bsd/impl/unistd/geteuid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/misc/getentropy.h>

#define min(a, b) ((a < b) ? a : b)

extern void* memcpy(void* dest, const void* src, __SIZE_TYPE__ n);

// global variable because we need to inherit this across child processes
static auditinfo_addr_t info = {
	.ai_auid = AU_DEFAUDITID,
	.ai_mask = {0},
	.ai_termid = { .at_type = AU_IPv4 },
	.ai_asid = AU_DEFAUDITSID,
	.ai_flags = 0,
};
// should be a rw lock but *shrug*
static libsimple_lock_t info_lock = LIBSIMPLE_LOCK_INITIALIZER;

// BSM audit condition, as auditon(2)'s GETCOND/SETCOND report it. Darling
// never writes audit records, so it starts out and stays AUC_DISABLED unless a
// caller sets it.
static int audit_condition = AUC_DISABLED;

long sys_getaudit_addr(struct auditinfo_addr* auditinfo_addr, int length) {
	libsimple_lock_lock(&info_lock);

	memcpy(auditinfo_addr, &info, min(length, sizeof(auditinfo_addr_t)));

	libsimple_lock_unlock(&info_lock);
	return 0;
};

long sys_setaudit_addr(struct auditinfo_addr* auditinfo_addr, int length) {
	libsimple_lock_lock(&info_lock);

	memcpy(&info, auditinfo_addr, min(length, sizeof(auditinfo_addr_t)));

	if (info.ai_asid == AU_ASSIGN_ASID) {
		// generate a new session ID
		sys_getentropy(&info.ai_asid, sizeof(info.ai_asid));
	}

	libsimple_lock_unlock(&info_lock);
	return 0;
};

void audit_session_set_mask(au_mask_t mask) {
	libsimple_lock_lock(&info_lock);
	info.ai_mask = mask;
	libsimple_lock_unlock(&info_lock);
}

void audit_session_set_auid(au_id_t auid) {
	libsimple_lock_lock(&info_lock);
	info.ai_auid = auid;
	libsimple_lock_unlock(&info_lock);
}

void audit_session_set_condition(int condition) {
	libsimple_lock_lock(&info_lock);
	audit_condition = condition;
	libsimple_lock_unlock(&info_lock);
}

int audit_session_get_condition(void) {
	libsimple_lock_lock(&info_lock);
	int condition = audit_condition;
	libsimple_lock_unlock(&info_lock);
	return condition;
}
