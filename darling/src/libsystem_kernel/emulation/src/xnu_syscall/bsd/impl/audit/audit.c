#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/conversion/fcntl/open.h>

#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/write.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fsync.h>

#include <stdint.h>
#include <bsm/audit.h>

/**
 * audit(2) appends a BSM record to the audit trail.
 *
 * The trail is a plain append-only file at /var/audit/trail inside the
 * container. The path is fixed rather than taken from the environment on
 * purpose: the launcher is installed setuid root and deliberately passes only a
 * small allowlist of variables to darlingserver, because the caller controls the
 * environment. A variable naming the trail would be exactly the kind of
 * file-selection input that allowlist exists to withhold.
 *
 * The path is handed to sys_open as a container path, not expanded to its host
 * location first. The kernel is confined to the prefix, so a host path does not
 * resolve and the open fails with ENOENT.
 *
 * A record counts as committed only once it is on disk: the write must consume
 * the whole record and fsync must succeed. login(1) treats a non-zero return
 * from au_close() as fatal and refuses to start a session, so returning 0
 * without writing the record would be the one dishonest option here.
 *
 * Scope: this retains records, which is the point of a BSM trail. It is not
 * tamper-evident. Real BSM has the kernel write a root-owned, size-limited,
 * rotated log; here the file belongs to whoever owns the container, so a
 * sufficiently privileged user can still edit it. Treat this as accounting, not
 * as an integrity guarantee.
 */

#define AUDIT_TRAIL_PATH "/var/audit/trail"

static int trail_fd = -1;

long sys_audit(void* record, int length) {
	if (record == NULL || length <= 0) {
		return -EINVAL;
	}

	if (trail_fd < 0) {
		// O_APPEND keeps concurrent writers from overwriting each other, and
		// 0600 keeps the trail readable only by the user that owns the
		// container.
		trail_fd = sys_open(AUDIT_TRAIL_PATH,
							BSD_O_WRONLY | BSD_O_APPEND | BSD_O_CREAT,
							0600);
		if (trail_fd < 0) {
			// No usable trail. Report this the way XNU does for a kernel
			// built without audit support rather than claiming success.
			return -ENOSYS;
		}
	}

	long written = sys_write(trail_fd, record, (size_t) length);
	if (written != (long) length) {
		return -EIO;
	}

	if (sys_fsync(trail_fd) < 0) {
		// The bytes are in the page cache but not durable, so the record is
		// not committed and the caller must be told so.
		return -EIO;
	}

	return 0;
}
