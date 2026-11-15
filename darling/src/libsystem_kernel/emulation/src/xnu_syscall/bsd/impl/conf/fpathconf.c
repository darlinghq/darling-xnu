#include <darling/emulation/xnu_syscall/bsd/impl/conf/fpathconf.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

enum {
	BSD_PC_LINK_MAX = 1,
	BSD_PC_MAX_CANON,
	BSD_PC_MAX_INPUT,
	BSD_PC_NAME_MAX,
	BSD_PC_PATH_MAX,
	BSD_PC_PIPE_BUF,
	BSD_PC_CHOWN_RESTRICTED,
	BSD_PC_NO_TRUNC,
	BSD_PC_VDISABLE,
	BSD_PC_NAME_CHARS_MAX,
	BSD_PC_CASE_SENSITIVE,
	BSD_PC_CASE_PRESERVING,
	BSD_PC_EXTENDED_SECURITY_NP,
	BSD_PC_AUTH_OPAQUE_NP,
	BSD_PC_2_SYMLINKS,
	BSD_PC_ALLOC_SIZE_MIN,
	BSD_PC_ASYNC_IO,
	BSD_PC_FILESIZEBITS,
	BSD_PC_PRIO_IO,
	BSD_PC_REC_INCR_XFER_SIZE,
	BSD_PC_REC_MAX_XFER_SIZE,
	BSD_PC_REC_MIN_XFER_SIZE,
	BSD_PC_REC_XFER_ALIGN,
	BSD_PC_SYMLINK_MAX,
	BSD_PC_SYNC_IO,
	BSD_PC_XATTR_SIZE_BITS,
	BSD_PC_MIN_HOLE_SIZE
};

// The _PC_ names 10 to 27 (sys/unistd.h) are answered the way macOS answers them for a path on an APFS volume.
// Observed with pathconf() and fpathconf() on a directory and a regular file on macOS 26.7.3 and 10.14.4
// (both APFS, black-box; the two agree except for names 18 and 26, which are 56 on 26.7.3 and 64 on 10.14.4: the
// wider value is used), and name 27 (_PC_MIN_HOLE_SIZE) too. Names above 27 are EINVAL on both.
static long pathconf_darwin_name(int name)
{
	switch (name) {
	case BSD_PC_NAME_CHARS_MAX: return 255;
	// The default APFS format is case insensitive and case preserving, and the vchroot lookup falls back to
	// case-insensitive matching (vchroot_userspace.c), so the guest namespace behaves the same.
	case BSD_PC_CASE_SENSITIVE: return 0;
	case BSD_PC_CASE_PRESERVING: return 1;
	// Deliberate difference: macOS reports 1 on APFS, but the guest has no access control lists, so the
	// truthful answer is 0.
	case BSD_PC_EXTENDED_SECURITY_NP: return 0;
	case BSD_PC_AUTH_OPAQUE_NP: return 0;
	case BSD_PC_2_SYMLINKS: return 1;
	case BSD_PC_ALLOC_SIZE_MIN: return 1;
	case BSD_PC_ASYNC_IO: return 1;
	case BSD_PC_FILESIZEBITS: return 64;
	case BSD_PC_PRIO_IO: return 0;
	case BSD_PC_REC_INCR_XFER_SIZE: return 4096;
	case BSD_PC_REC_MAX_XFER_SIZE: return 65536;
	case BSD_PC_REC_MIN_XFER_SIZE: return 4096;
	case BSD_PC_REC_XFER_ALIGN: return 4096;
	case BSD_PC_SYMLINK_MAX: return 255;
	case BSD_PC_SYNC_IO: return 0;
	case BSD_PC_XATTR_SIZE_BITS: return 64;
	case BSD_PC_MIN_HOLE_SIZE: return 4096;
	default: return -EINVAL;
	}
}

long sys_fpathconf(int fd, int name)
{
	static const short values[] = {
		[BSD_PC_LINK_MAX] = 8,
		[BSD_PC_MAX_CANON] = 255,
		[BSD_PC_MAX_INPUT] = 255,
		[BSD_PC_NAME_MAX] = 255,
		[BSD_PC_PATH_MAX] = 4096,
		[BSD_PC_PIPE_BUF] = 4096,
		[BSD_PC_CHOWN_RESTRICTED] = 1,
		[BSD_PC_NO_TRUNC] = 1,
		[BSD_PC_VDISABLE] = 0
	};
	
	if (name >= BSD_PC_NAME_CHARS_MAX)
		return pathconf_darwin_name(name);
	if (name < 1 || name >= sizeof(values) / sizeof(values[0]))
		return -EINVAL;
	else
		return values[name];
}
