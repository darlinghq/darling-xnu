#ifndef DARLING_BSDFLAGS_H
#define DARLING_BSDFLAGS_H
// BSD file flags (chflags: UF_TRACKED, UF_HIDDEN, ...) kept in the Linux extended attribute
// "user.x86code.bsdflags" (a little-endian u32; absent = 0), and reported as st_flags by stat and ATTR_CMN_FLAGS by
// getattrlist. Linux has no such flags (its FS_IOC_SETFLAGS ones are different and mostly need root). Darling's
// chflags/fchflags failed with ENOTSUP, and APFS's FSIOC_CAS_BSDFLAGS fsctl was missing: FileProvider's libfssync
// sets UF_TRACKED on every item of the On My iPhone storage and trapped ("call to block failed with -1"), so Files
// could open no document. Nothing acts on a flag (UF_IMMUTABLE doesn't stop writes); they are bookkeeping, as on a
// file system that stores but doesn't enforce them. Symlinks can't have user xattrs on Linux: their flags are 0.
// The attribute is hidden from listxattr (xattr_utils.h).
#include <stdint.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define DARLING_BSDFLAGS_XATTR "user.x86code.bsdflags"

// Linux path of fd (works for O_PATH descriptors too), in buf (>= 32 bytes)
static inline const char* darling_bsdflags_fd_path(int fd, char* buf)
{
	__simple_sprintf(buf, "/proc/self/fd/%d", fd);
	return buf;
}

static inline uint32_t darling_bsdflags_bsdflags_get(const char* lpath, int nofollow)
{
	uint32_t v = 0;
	long r = LINUX_SYSCALL(nofollow ? __NR_lgetxattr : __NR_getxattr, lpath, DARLING_BSDFLAGS_XATTR, &v, sizeof(v));
	return r == sizeof(v) ? v : 0;
}

// path relative to dfd (Linux AT_FDCWD = -100 or a descriptor), as fstatat takes it
static inline uint32_t darling_bsdflags_bsdflags_get_at(int dfd, const char* path, int nofollow)
{
	if (path[0] == '/' || dfd == -100)
		return darling_bsdflags_bsdflags_get(path, nofollow);
	char buf[1100];
	__simple_snprintf(buf, sizeof(buf), "/proc/self/fd/%d/%s", dfd, path);
	return darling_bsdflags_bsdflags_get(buf, nofollow);
}

static inline uint32_t darling_bsdflags_bsdflags_get_fd(int fd)
{
	char buf[32];
	return darling_bsdflags_bsdflags_get(darling_bsdflags_fd_path(fd, buf), 0);
}

// returns 0 or a negative Linux errno
static inline long darling_bsdflags_bsdflags_set(const char* lpath, uint32_t v)
{
	if (v == 0)
	{
		long r = LINUX_SYSCALL(__NR_removexattr, lpath, DARLING_BSDFLAGS_XATTR);
		return (r == 0 || r == -61 /* ENODATA */) ? 0 : r;
	}
	return LINUX_SYSCALL(__NR_setxattr, lpath, DARLING_BSDFLAGS_XATTR, &v, sizeof(v), 0);
}

#endif
