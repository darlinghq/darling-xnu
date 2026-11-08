#include <darling/emulation/xnu_syscall/bsd/impl/misc/csops.h>

#include <stdint.h>
#include <string.h>
#include <sys/codesign.h>
#include <sys/errno.h>
#include <sys/signal.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/signal/kill.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getpid.h>

#include <darlingserver/rpc.h>

// csops() answers from the code signing state darlingserver records for each
// process (see `dserver_cs_state_t`). The operations that change flags are
// carried out there too, so every process sees the same state.

// the flags CS_OPS_SET_STATUS may add
#define CS_SET_STATUS_FLAGS (CS_HARD | CS_EXEC_SET_HARD | CS_KILL | CS_EXEC_SET_KILL | \
	CS_RESTRICT | CS_REQUIRE_LV | CS_ENFORCEMENT | CS_EXEC_SET_ENFORCEMENT)

// what CS_OPS_CLEARINSTALLER removes
#define CS_INSTALLER_FLAGS (CS_INSTALLER | CS_DATAVAULT_CONTROLLER | CS_EXEC_INHERIT_SIP)

// the size of the header csops() puts in front of blobs and identities
#define CS_BLOB_HEADER_SIZE 8

int cs_get_state(int pid, dserver_cs_state_t* state) {
	int ret = dserver_rpc_cs_get_state(pid, state, sizeof(*state));
	if (ret < 0)
		return errno_linux_to_bsd(ret);
	return 0;
}

uint32_t cs_own_policy(void) {
	// the policy plus one; 0 means it has not been asked for yet
	static uint32_t cached = 0;
	uint32_t value = __atomic_load_n(&cached, __ATOMIC_RELAXED);

	if (value == 0) {
		dserver_cs_state_t state;
		if (cs_get_state(0, &state) != 0)
			return dserver_code_signing_policy_off;
		value = state.code_signing_policy + 1;
		__atomic_store_n(&cached, value, __ATOMIC_RELAXED);
	}

	return value - 1;
}

static int update_flags(int pid, uint32_t set_flags, uint32_t clear_flags) {
	uint32_t flags = 0;
	int ret = dserver_rpc_cs_update_flags(pid, set_flags, clear_flags, &flags);
	if (ret < 0)
		return errno_linux_to_bsd(ret);
	return 0;
}

// The flags as CS_OPS_STATUS reports them.
static uint32_t status_flags(const dserver_cs_state_t* state) {
	uint32_t flags;

	if (state->status == dserver_cs_status_unregistered) {
		// Nothing is known about the image the process runs (no image has
		// been registered for it): keep reporting it as valid.
		flags = CS_VALID;
	} else {
		flags = state->flags;
	}

	// Linux lets any process map writable+executable memory, so tell
	// apps what iOS tells a process with a debugger attached: CS_DEBUGGED
	// is the bit JIT-using apps (emulators, UTM) check before they JIT.
	flags |= CS_DEBUGGED;

	// library validation the system turned on (CS_FORCED_LV) is not also
	// reported as requested by the process
	if (flags & CS_FORCED_LV)
		flags &= ~CS_REQUIRE_LV;

	return flags;
}

static int valid_or_debugged(const dserver_cs_state_t* state) {
	return (status_flags(state) & (CS_VALID | CS_DEBUGGED)) != 0;
}

// An invalid CS_KILL process is killed only when its policy enforces code
// signing; otherwise CS_KILL is just reported.
static void kill_invalid(int pid, const dserver_cs_state_t* state) {
	if (state->code_signing_policy != dserver_code_signing_policy_enforce)
		return;
	update_flags(pid, CS_KILLED, 0);
	sys_kill(pid ? pid : sys_getpid(), SIGKILL, 1);
}

static void put_header(void* useraddr, uint32_t length) {
	uint8_t header[CS_BLOB_HEADER_SIZE] = { 0 };

	// the length is big-endian
	header[4] = (uint8_t)(length >> 24);
	header[5] = (uint8_t)(length >> 16);
	header[6] = (uint8_t)(length >> 8);
	header[7] = (uint8_t)length;
	memcpy(useraddr, header, sizeof(header));
}

// An identity is copied after a header whose length covers the header and
// the string with its NUL.
static int copy_identity(const char* identity, size_t capacity, void* useraddr, size_t usersize) {
	size_t length = 0;

	while (length < capacity && identity[length] != '\0')
		length++;

	if (length == 0 || length == capacity)
		return -ENOENT;
	if (useraddr == NULL)
		return -EFAULT;

	put_header(useraddr, (uint32_t)(CS_BLOB_HEADER_SIZE + length + 1));
	if (usersize < CS_BLOB_HEADER_SIZE + length + 1)
		return -ERANGE;

	memcpy((uint8_t*)useraddr + CS_BLOB_HEADER_SIZE, identity, length + 1);
	return 0;
}

static int copy_entitlements(int pid, const dserver_cs_state_t* state, void* useraddr, size_t usersize) {
	uint64_t length = 0;
	int ret;

	if (usersize < CS_BLOB_HEADER_SIZE)
		return -ERANGE;

	// QUIRK: without entitlements the call succeeds and leaves the buffer
	// as it is; no empty blob header is written, whatever the buffer size.
	// Evidence: black-box observation on macOS 27.0.1 (arm64; ad-hoc and
	// linker-signed programs; tier 2). With entitlements, an 8-byte buffer
	// gets ERANGE and a header with the length, as documented.
	// Test: test_quirk_no_entitlements_leaves_buffer in cs_csops.c.
	if (state->entitlements_length == 0)
		return 0;

	if (useraddr == NULL)
		return -EFAULT;
	if (usersize < state->entitlements_length) {
		put_header(useraddr, state->entitlements_length);
		return -ERANGE;
	}

	ret = dserver_rpc_cs_copy_entitlements(pid, false, useraddr, usersize, &length);
	if (ret < 0)
		return errno_linux_to_bsd(ret);
	return 0;
}

static long csops_common(int pid, unsigned int ops, void* useraddr, size_t usersize) {
	dserver_cs_state_t state;
	int ret;

	// darlingserver takes 0 for the calling process
	if (pid == sys_getpid())
		pid = 0;

	ret = cs_get_state(pid, &state);
	if (ret != 0)
		return ret;

	switch (ops) {
		case CS_OPS_STATUS: {
			uint32_t flags = status_flags(&state);
			// copied out whatever the size given, as on macOS
			if (useraddr != NULL)
				memcpy(useraddr, &flags, sizeof(flags));
			return 0;
		}

		case CS_OPS_MARKINVALID:
			if (!(status_flags(&state) & CS_VALID))
				return 0;
			ret = update_flags(pid, 0, CS_VALID);
			if (ret == 0 && (state.flags & CS_KILL))
				kill_invalid(pid, &state);
			return ret;

		case CS_OPS_MARKHARD:
			ret = update_flags(pid, CS_HARD, 0);
			if (ret == 0 && !(status_flags(&state) & CS_VALID))
				return -EINVAL;
			return ret;

		case CS_OPS_MARKKILL:
			ret = update_flags(pid, CS_KILL, 0);
			if (ret == 0 && !(status_flags(&state) & CS_VALID))
				kill_invalid(pid, &state);
			return ret;

		case CS_OPS_MARKRESTRICT:
			return update_flags(pid, CS_RESTRICT, 0);

		case CS_OPS_PIDOFFSET: {
			int64_t offset = (int64_t)state.image_offset;
			if (useraddr == NULL)
				return -EFAULT;
			memcpy(useraddr, &offset, sizeof(offset));
			return 0;
		}

		case CS_OPS_CDHASH:
			if (usersize != sizeof(state.cdhash) || state.status == dserver_cs_status_unregistered)
				return -EINVAL;
			// no signature covers the image
			if (state.status != dserver_cs_status_valid)
				return -EBADEXEC;
			if (useraddr == NULL)
				return -EFAULT;
			memcpy(useraddr, state.cdhash, sizeof(state.cdhash));
			return 0;

		case CS_OPS_ENTITLEMENTS_BLOB:
			if (!valid_or_debugged(&state))
				return -EINVAL;
			return copy_entitlements(pid, &state, useraddr, usersize);

		case CS_OPS_SET_STATUS: {
			uint32_t flags;
			if (usersize < sizeof(flags))
				return -ERANGE;
			if (useraddr == NULL)
				return -EFAULT;
			memcpy(&flags, useraddr, sizeof(flags));
			if (!(status_flags(&state) & CS_VALID))
				return -EINVAL;
			return update_flags(pid, flags & CS_SET_STATUS_FLAGS, 0);
		}

		case CS_OPS_BLOB:
			if (!valid_or_debugged(&state))
				return -EINVAL;
			if (usersize < CS_BLOB_HEADER_SIZE)
				return -ERANGE;
			if (useraddr == NULL)
				return -EFAULT;
			// darlingserver does not hand out the signature itself; answer
			// as for a process without one, with an empty header
			put_header(useraddr, 0);
			return 0;

		case CS_OPS_IDENTITY:
		case CS_OPS_TEAMID:
			if (usersize < CS_BLOB_HEADER_SIZE)
				return -ERANGE;
			if (!valid_or_debugged(&state))
				return -EINVAL;
			if (ops == CS_OPS_TEAMID)
				return copy_identity(state.team_id, sizeof(state.team_id), useraddr, usersize);
			return copy_identity(state.identifier, sizeof(state.identifier), useraddr, usersize);

		case CS_OPS_CLEARINSTALLER:
			return update_flags(pid, 0, CS_INSTALLER_FLAGS);

		case CS_OPS_CLEARPLATFORM:
			// only development kernels support this
			return -ENOTSUP;

		case CS_OPS_CLEAR_LV:
			// a process can only turn off its own library validation
			if (pid != 0)
				return -EPERM;
			return update_flags(0, 0, CS_REQUIRE_LV | CS_FORCED_LV);

		default:
			return -EINVAL;
	}
}

long sys_csops(int pid, unsigned int ops, void* useraddr, size_t usersize) {
	return csops_common(pid, ops, useraddr, usersize);
}

long sys_csops_audittoken(int pid, unsigned int ops, void* useraddr, size_t usersize, audit_token_t* audit_token) {
	if (audit_token == NULL)
		return -EINVAL;

	if (pid == 0)
		pid = sys_getpid();

	// The token must name the process. Its PID version (val[7]) is not
	// checked, because darlingserver does not track PID versions.
	if (audit_token->val[5] != (unsigned int)pid)
		return -ESRCH;

	return csops_common(pid, ops, useraddr, usersize);
}
