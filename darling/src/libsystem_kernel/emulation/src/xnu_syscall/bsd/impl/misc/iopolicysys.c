#include <darling/emulation/xnu_syscall/bsd/impl/misc/iopolicysys.h>

#include <sys/errno.h>
#include <stddef.h>
#include <stdint.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/tsd.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/geteuid.h>

// I/O policies: the process scoped policies are kept in this image (a fork copies them), the thread scoped ones in a
// thread specific value. Nothing is enforced; the values are only remembered so that a get returns what a set stored.
// The accepted scopes, policies and read-back values are what macOS 26.7.3 answers to setiopolicy_np() and
// getiopolicy_np() for each type (black-box, see the test), where 10.14 has the type at all.
//
// Known differences from macOS:
// - Darling has no entitlements. macOS lets root set case sensitivity, statfs and content protection policies only
//   with root or an entitlement, and permissions and mtime ones only with an entitlement; here root may set all of
//   them and everybody else gets EPERM (the answer observed for a non-root process).
// - An exec resets the process scoped state (this image is reloaded).
// - The disk type does not model the interaction with QoS (EIDRM).
// - Types above IOPOL_TYPE_VFS_SKIP_MTIME_UPDATE are EINVAL.

// struct _iopol_param_t and the values of xnu bsd/sys/resource.h
struct iopol_param {
	int iop_scope;
	int iop_iotype;
	int iop_policy;
};

#define IOPOL_CMD_GET 1
#define IOPOL_CMD_SET 2

#define SCOPE_PROCESS 0
#define SCOPE_THREAD 1
#define SCOPE_DARWIN_BG 2

#define TYPE_DISK 0
#define TYPE_CASE_SENSITIVITY 1
#define TYPE_ATIME_UPDATES 2
#define TYPE_MATERIALIZE_DATALESS_FILES 3
#define TYPE_STATFS_NO_DATA_VOLUME 4
#define TYPE_TRIGGER_RESOLVE 5
#define TYPE_IGNORE_CONTENT_PROTECTION 6
#define TYPE_IGNORE_PERMISSIONS 7
#define TYPE_SKIP_MTIME_UPDATE 8

#define DISK_DEFAULT 0
#define DISK_IMPORTANT 1
#define DISK_PASSIVE 2
#define DISK_THROTTLE 3
#define DISK_UTILITY 4
#define DISK_STANDARD 5

#define MATERIALIZE_DEFAULT 0
#define MATERIALIZE_OFF 1
#define MATERIALIZE_ON 2

// Process scoped state: one flag per two-valued type (the flag set means the non-default value), the process's
// materialization policy and the disk policies of the process and of IOPOL_SCOPE_DARWIN_BG. A disk policy is kept as
// it reads back: IOPOL_IMPORTANT and IOPOL_DEFAULT are both the default tier and read back as IOPOL_DEFAULT.
// A process that has not set the materialization policy reads IOPOL_MATERIALIZE_DATALESS_FILES_ON; setting
// IOPOL_MATERIALIZE_DATALESS_FILES_DEFAULT for the process reads back as OFF.
#define PROCESS_MATERIALIZE_OFF (1u << 8)
static volatile uint32_t process_flags;
static volatile int process_disk = DISK_DEFAULT;
static volatile int process_background_disk = DISK_UTILITY; // xnu: the background scope reads UTILITY unless set

// Thread scoped state, packed into one thread specific word (0 in every field is the default).
#define THREAD_DISK_SHIFT 0 // 3 bits, 0 means "not set by this thread"
#define THREAD_ATIME_BIT (1u << 3)
#define THREAD_MATERIALIZE_SHIFT 4 // 2 bits, a MATERIALIZE_* value
#define THREAD_PERMISSIONS_BIT (1u << 6)
#define THREAD_MTIME_BIT (1u << 7)

static inline uintptr_t thread_state(void)
{
	return (uintptr_t) _pthread_getspecific_direct(__PTK_DARLING_IOPOLICY_THREAD);
}

static inline void set_thread_state(uintptr_t state)
{
	_pthread_setspecific_direct(__PTK_DARLING_IOPOLICY_THREAD, (void*) state);
}

static inline int is_root(void)
{
	return sys_geteuid() == 0; // xnu: kauth_cred_issuser()
}

static long disk(int cmd, int scope, int* policy)
{
	if (scope != SCOPE_PROCESS && scope != SCOPE_THREAD && scope != SCOPE_DARWIN_BG)
		return -EINVAL;
	if (cmd == IOPOL_CMD_SET) {
		switch (*policy) {
			case DISK_DEFAULT:
				// the default of the background scope is the utility tier, of the others tier 0 (reads back as DEFAULT)
				*policy = (scope == SCOPE_DARWIN_BG) ? DISK_UTILITY : DISK_DEFAULT;
				break;
			case DISK_UTILITY:
			case DISK_THROTTLE:
				break;
			case DISK_IMPORTANT:
			case DISK_STANDARD:
			case DISK_PASSIVE:
				if (scope == SCOPE_DARWIN_BG)
					return -EINVAL;
				if (*policy == DISK_IMPORTANT)
					*policy = DISK_DEFAULT; // tier 0
				break;
			default:
				return -EINVAL;
		}
		if (scope == SCOPE_THREAD)
			set_thread_state((thread_state() & ~(7u << THREAD_DISK_SHIFT)) | ((uintptr_t) *policy << THREAD_DISK_SHIFT));
		else if (scope == SCOPE_DARWIN_BG)
			process_background_disk = *policy;
		else
			process_disk = *policy;
		return 0;
	}
	if (cmd != IOPOL_CMD_GET)
		return -EINVAL;
	if (scope == SCOPE_THREAD) {
		// a thread's own value, never the process's (xnu proc_get_thread_policy)
		*policy = (thread_state() >> THREAD_DISK_SHIFT) & 7;
	} else {
		*policy = (scope == SCOPE_DARWIN_BG) ? process_background_disk : process_disk;
	}
	return 0;
}

// The types kept as one two-valued flag, some also per thread: `flag` is the process flag, `thread_bit` the thread flag
// (0: no thread scope) and `privileged` marks the ones only root may set (see above). The order of the checks is what
// macOS does for a non-root process: a bad scope is EINVAL first; the permissions type then refuses with EPERM before it
// looks at the policy, the others look at the policy (EINVAL) first.
struct flag_type {
	uint32_t flag;
	uintptr_t thread_bit;
	int privileged;
	int privilege_before_policy;
};

static const struct flag_type case_sensitivity = { 1u << 0, 0, 1, 0 };
static const struct flag_type atime_updates = { 1u << 1, THREAD_ATIME_BIT, 0, 0 };
static const struct flag_type statfs_no_data_volume = { 1u << 2, 0, 1, 0 };
static const struct flag_type trigger_resolve = { 1u << 3, 0, 0, 0 };
static const struct flag_type ignore_content_protection = { 1u << 4, 0, 1, 0 };
static const struct flag_type ignore_permissions = { 1u << 5, THREAD_PERMISSIONS_BIT, 1, 1 };
static const struct flag_type skip_mtime_update = { 1u << 6, THREAD_MTIME_BIT, 1, 0 };

static long flag_policy(int cmd, int scope, int* policy, const struct flag_type* type)
{
	if (scope != SCOPE_PROCESS && !(type->thread_bit != 0 && scope == SCOPE_THREAD))
		return -EINVAL;
	if (cmd == IOPOL_CMD_SET) {
		if (type->privilege_before_policy && type->privileged && !is_root())
			return -EPERM;
		if (*policy != 0 && *policy != 1)
			return -EINVAL;
		if (type->privileged && !is_root())
			return -EPERM;
		if (scope == SCOPE_THREAD)
			set_thread_state(*policy ? (thread_state() | type->thread_bit) : (thread_state() & ~type->thread_bit));
		else if (*policy)
			__atomic_or_fetch(&process_flags, type->flag, __ATOMIC_RELAXED);
		else
			__atomic_and_fetch(&process_flags, ~type->flag, __ATOMIC_RELAXED);
		return 0;
	}
	if (cmd != IOPOL_CMD_GET)
		return -EINVAL;
	if (scope == SCOPE_THREAD)
		*policy = (thread_state() & type->thread_bit) ? 1 : 0;
	else
		*policy = (process_flags & type->flag) ? 1 : 0;
	return 0;
}

static long materialize(int cmd, int scope, int* policy)
{
	if (scope != SCOPE_PROCESS && scope != SCOPE_THREAD)
		return -EINVAL;
	if (cmd == IOPOL_CMD_SET) {
		if (*policy != MATERIALIZE_DEFAULT && *policy != MATERIALIZE_OFF && *policy != MATERIALIZE_ON)
			return -EINVAL;
		if (scope == SCOPE_THREAD)
			set_thread_state((thread_state() & ~(3u << THREAD_MATERIALIZE_SHIFT)) | ((uintptr_t) *policy << THREAD_MATERIALIZE_SHIFT));
		else if (*policy == MATERIALIZE_ON)
			__atomic_and_fetch(&process_flags, ~PROCESS_MATERIALIZE_OFF, __ATOMIC_RELAXED);
		else // OFF, and DEFAULT too
			__atomic_or_fetch(&process_flags, PROCESS_MATERIALIZE_OFF, __ATOMIC_RELAXED);
		return 0;
	}
	if (cmd != IOPOL_CMD_GET)
		return -EINVAL;
	if (scope == SCOPE_THREAD)
		*policy = (thread_state() >> THREAD_MATERIALIZE_SHIFT) & 3; // a thread without its own setting inherits: DEFAULT
	else
		*policy = (process_flags & PROCESS_MATERIALIZE_OFF) ? MATERIALIZE_OFF : MATERIALIZE_ON;
	return 0;
}

long sys_iopolicysys(int cmd, void* arg)
{
	struct iopol_param* param = arg;
	if (param == NULL)
		return -EFAULT;

	int policy = param->iop_policy;
	long result;
	switch (param->iop_iotype) {
		case TYPE_DISK:
			result = disk(cmd, param->iop_scope, &policy);
			break;
		case TYPE_CASE_SENSITIVITY:
			result = flag_policy(cmd, param->iop_scope, &policy, &case_sensitivity);
			break;
		case TYPE_ATIME_UPDATES:
			result = flag_policy(cmd, param->iop_scope, &policy, &atime_updates);
			break;
		case TYPE_MATERIALIZE_DATALESS_FILES:
			result = materialize(cmd, param->iop_scope, &policy);
			break;
		case TYPE_STATFS_NO_DATA_VOLUME:
			result = flag_policy(cmd, param->iop_scope, &policy, &statfs_no_data_volume);
			break;
		case TYPE_TRIGGER_RESOLVE:
			result = flag_policy(cmd, param->iop_scope, &policy, &trigger_resolve);
			break;
		case TYPE_IGNORE_CONTENT_PROTECTION:
			result = flag_policy(cmd, param->iop_scope, &policy, &ignore_content_protection);
			break;
		case TYPE_IGNORE_PERMISSIONS:
			result = flag_policy(cmd, param->iop_scope, &policy, &ignore_permissions);
			break;
		case TYPE_SKIP_MTIME_UPDATE:
			result = flag_policy(cmd, param->iop_scope, &policy, &skip_mtime_update);
			break;
		default:
			return -EINVAL;
	}
	if (result == 0 && cmd == IOPOL_CMD_GET)
		param->iop_policy = policy;
	return result;
}
