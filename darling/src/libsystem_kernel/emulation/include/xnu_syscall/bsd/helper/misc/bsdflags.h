#ifndef DARLING_BSDFLAGS_H
#define DARLING_BSDFLAGS_H
// BSD file flags (chflags(2): UF_NODUMP, UF_HIDDEN, UF_TRACKED, ...) kept in the Linux extended attribute
// "user.darling.bsdflags" (a native-endian u32; absent = 0), and reported as st_flags by stat and ATTR_CMN_FLAGS by
// getattrlist. Linux has no such flags (its FS_IOC_SETFLAGS ones are different and mostly need root), so chflags
// used to fail with ENOTSUP. Nothing acts on a flag (UF_IMMUTABLE does not stop writes): they are bookkeeping, as on
// a file system that stores but does not enforce them. Only regular files and directories can carry a user xattr on
// Linux; a symlink, or a file system without user xattrs, reports 0 and refuses a change.
// The attribute is hidden from listxattr (xattr_utils.h).
#include <stdint.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define DARLING_BSDFLAGS_XATTR "user.darling.bsdflags"

// Linux path of fd (works for O_PATH descriptors too), in buf (>= 32 bytes)
static inline const char* darling_bsdflags_fd_path(int fd, char* buf)
{
	__simple_sprintf(buf, "/proc/self/fd/%d", fd);
	return buf;
}

static inline uint32_t darling_bsdflags_get(const char* lpath, int nofollow)
{
	uint32_t v = 0;
	long r = LINUX_SYSCALL(nofollow ? __NR_lgetxattr : __NR_getxattr, lpath, DARLING_BSDFLAGS_XATTR, &v, sizeof(v));
	return r == sizeof(v) ? v : 0;
}

// path relative to dfd (Linux AT_FDCWD = -100 or a descriptor), as fstatat takes it
static inline uint32_t darling_bsdflags_get_at(int dfd, const char* path, int nofollow)
{
	if (path[0] == '/' || dfd == -100)
		return darling_bsdflags_get(path, nofollow);
	char buf[1100];
	__simple_snprintf(buf, sizeof(buf), "/proc/self/fd/%d/%s", dfd, path);
	return darling_bsdflags_get(buf, nofollow);
}

static inline uint32_t darling_bsdflags_get_fd(int fd)
{
	char buf[32];
	return darling_bsdflags_get(darling_bsdflags_fd_path(fd, buf), 0);
}

// returns 0 or a negative Linux errno
static inline long darling_bsdflags_set(const char* lpath, uint32_t v)
{
	if (v == 0)
	{
		long r = LINUX_SYSCALL(__NR_removexattr, lpath, DARLING_BSDFLAGS_XATTR);
		// nothing to clear when there is no attribute (ENODATA) or the file system has no user xattrs (ENOTSUP)
		return (r == 0 || r == -61 /* ENODATA */ || r == -95 /* EOPNOTSUPP */) ? 0 : r;
	}
	return LINUX_SYSCALL(__NR_setxattr, lpath, DARLING_BSDFLAGS_XATTR, &v, sizeof(v), 0);
}

#endif
