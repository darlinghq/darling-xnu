#include <darling/emulation/xnu_syscall/bsd/impl/misc/fsctl.h>

#include <sys/errno.h>
#include <stddef.h>
#include <stdint.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstatat.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/bsdflags.h>

// After xnu bsd/vfs/vfs_syscalls.c fsctl()/ffsctl() and fsctl_internal(). Every command that is not handled by the
// VFS layer itself is passed to the file system (VNOP_IOCTL), and no file system a Darling guest sees implements one
// (the volumes are Linux file systems), so those fail with ENOTTY, the error xnu gives a file system without the
// command. The commands xnu answers in the VFS layer (FSIOC_SYNC_VOLUME and the like) are not implemented.
//
// The guest's volumes report themselves as APFS (see statfs), so the requests APFS answers and that a program
// meets on any volume are answered for a plain tree of files: see fsctl_apfs_request().

#define F_GETFD_COMMAND 1
#define FSOPT_NOFOLLOW 0x00000001 // bsd/sys/attr.h
#define BSD_AT_SYMLINK_NOFOLLOW 0x0020
// _IOWR('A', 21, uint32_t) of bsd/sys/fsctl.h
#define FSIOC_FD_ONLY_OPEN_ONCE 0xC0044115UL

// _IOWR('A', 20, struct fsioc_cas_bsdflags) of bsd/sys/fsctl.h: replace the file's chflags flags with new_flags if
// they are expected_flags now; the flags found are returned in actual_flags either way, and a mismatch is not an
// error (observed on macOS 26.7.3: the call succeeds and changes nothing).
#define FSIOC_CAS_BSDFLAGS 0xC00C4114UL
struct fsioc_cas_bsdflags
{
	unsigned int expected_flags;
	unsigned int new_flags;
	unsigned int actual_flags;
};

// APFS requests (not in a public header). Both take a 64-bit value.
//
// _IOR('J', 74, uint64_t), APFSIOC_GET_PARENT_SYNCROOT: the id of the sync root (File Provider) that contains the
// object. A tree without sync roots has none: the value is 0.
// Observed on macOS 26.7.3 (APFS): a file, a directory, a symbolic link (followed or, with FSOPT_NOFOLLOW, not) and a
// descriptor of each give 0 and set the value to 0; the root of a volume is EINVAL and leaves the value alone; a
// missing path is ENOENT, and a NULL data pointer EFAULT (checked after the path).
#define APFSIOC_GET_PARENT_SYNCROOT 0x40084a4aUL
// _IOW('J', 2, uint64_t), APFSIOC_MAINTAIN_DIR_STATS: asks APFS to keep the directory's size statistics up to date.
// A file system that keeps no such statistics has nothing to do. Observed on macOS 26.7.3: a directory (path or
// descriptor) with the value 0 gives 0; a file gives EINVAL; a missing path ENOENT; a NULL data pointer EFAULT, also
// for a file (the pointer is checked before the kind of object). Other values were not tried: the call did not return
// for an arbitrary one.
#define APFSIOC_MAINTAIN_DIR_STATS 0x80084a02UL

// Linux statx (fixed 256-byte ABI), the fields used
struct fsctl_statx
{
	uint32_t mask;
	uint32_t blksize;
	uint64_t attributes;
	uint32_t nlink;
	uint32_t uid;
	uint32_t gid;
	uint16_t mode;
	uint16_t spare0;
	uint64_t ino;
	uint64_t size;
	uint64_t blocks;
	uint64_t attributes_mask;
	unsigned char times[64];
	uint32_t rdev_major, rdev_minor;
	uint32_t dev_major, dev_minor;
	unsigned char remainder[112];
};
_Static_assert(sizeof(struct fsctl_statx) == 256, "Linux statx size");
_Static_assert(offsetof(struct fsctl_statx, dev_major) == 136, "Linux statx device offset");

#define FSCTL_STATX_BASIC_STATS 0x7ffU
#define FSCTL_STATX_ATTR_MOUNT_ROOT 0x2000ULL
#define FSCTL_AT_EMPTY_PATH 0x1000

#define FSCTL_S_IFMT 0170000
#define FSCTL_S_IFDIR 0040000

// lpath: Linux path of the file (also an O_PATH descriptor's /proc/self/fd path)
static long fsctl_cas_bsdflags(const char* lpath, void* data)
{
	struct fsioc_cas_bsdflags* cas = data;
	if (cas == NULL)
		return -EFAULT;
	unsigned int cur = darling_bsdflags_get(lpath, 0);
	if (cur == cas->expected_flags && cur != cas->new_flags)
	{
		long rv = darling_bsdflags_set(lpath, cas->new_flags);
		if (rv < 0)
			return errno_linux_to_bsd(rv);
	}
	cas->actual_flags = cur;
	return 0;
}

static long fsctl_statx(int dfd, const char* lpath, int flags, struct fsctl_statx* st)
{
	long rv = LINUX_SYSCALL(__NR_statx, dfd, lpath, flags, FSCTL_STATX_BASIC_STATS, st);
	return rv < 0 ? errno_linux_to_bsd(rv) : 0;
}

// Whether the object is the root of a volume: the root of a mount, or the root of the guest's tree (which is a
// directory of the host's file system when the prefix is not a mount of its own, and is "/" to the guest either way).
static long fsctl_is_volume_root(const struct fsctl_statx* object, int* is_root)
{
	*is_root = 0;
	if ((object->mode & FSCTL_S_IFMT) != FSCTL_S_IFDIR)
		return 0;
	if (object->attributes & object->attributes_mask & FSCTL_STATX_ATTR_MOUNT_ROOT)
	{
		*is_root = 1;
		return 0;
	}

	struct vchroot_expand_args guest;
	struct fsctl_statx root;
	guest.flags = VCHROOT_FOLLOW;
	guest.dfd = -100;
	strcpy(guest.path, "/");
	long rv = vchroot_expand(&guest);
	if (rv < 0)
		return errno_linux_to_bsd(rv);
	rv = fsctl_statx(-100, guest.path, 0, &root);
	if (rv < 0)
		return rv;
	*is_root = root.dev_major == object->dev_major && root.dev_minor == object->dev_minor && root.ino == object->ino;
	return 0;
}

// The APFS requests, for the object at (dfd, lpath, flags) of statx. Returns -ENOTTY for any other request.
static long fsctl_apfs_request(unsigned long cmd, void* data, int dfd, const char* lpath, int flags)
{
	struct fsctl_statx object;
	long rv;

	switch (cmd & 0xffffffffUL)
	{
		case APFSIOC_GET_PARENT_SYNCROOT:
		{
			int root;
			if (data == NULL)
				return -EFAULT;
			rv = fsctl_statx(dfd, lpath, flags, &object);
			if (rv < 0)
				return rv;
			rv = fsctl_is_volume_root(&object, &root);
			if (rv < 0)
				return rv;
			if (root)
				return -EINVAL;
			*((uint64_t*) data) = 0;
			return 0;
		}
		case APFSIOC_MAINTAIN_DIR_STATS:
			if (data == NULL)
				return -EFAULT;
			rv = fsctl_statx(dfd, lpath, flags, &object);
			if (rv < 0)
				return rv;
			return (object.mode & FSCTL_S_IFMT) == FSCTL_S_IFDIR ? 0 : -EINVAL;
		default:
			return -ENOTTY;
	}
}

static int fsctl_is_answered(unsigned long cmd)
{
	switch (cmd & 0xffffffffUL)
	{
		case FSIOC_CAS_BSDFLAGS:
		case APFSIOC_GET_PARENT_SYNCROOT:
		case APFSIOC_MAINTAIN_DIR_STATS:
			return 1;
		default:
			return 0;
	}
}

long sys_fsctl(const char* path, unsigned long cmd, void* data, unsigned int options)
{
	// by definition the file is not open through fsctl(), so "am I the only opener" is refused
	if (cmd == FSIOC_FD_ONLY_OPEN_ONCE)
		return -EINVAL;
	if (path == NULL)
		return -EFAULT;
	// xnu follows a symbolic link at the end of the path unless FSOPT_NOFOLLOW is given
	// darling_stat64_t is an incomplete type in the emulation headers (<sys/stat.h> cannot be included here), and the result
	// is not needed: a buffer larger than the BSD struct stat (144 bytes) receives it
	char info[256] __attribute__((aligned(8)));
	long error = sys_fstatat64(get_perthread_wd(), path, (darling_stat64_t*) info, (options & FSOPT_NOFOLLOW) ? BSD_AT_SYMLINK_NOFOLLOW : 0);
	if (error < 0)
		return error;

	if (!fsctl_is_answered(cmd))
		return -ENOTTY;

	struct vchroot_expand_args vc;
	vc.flags = (options & FSOPT_NOFOLLOW) ? 0 : VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();
	strcpy(vc.path, path);
	long rv = vchroot_expand(&vc);
	if (rv < 0)
		return errno_linux_to_bsd(rv);

	if ((cmd & 0xffffffffUL) == FSIOC_CAS_BSDFLAGS)
	{
		char buf[1100];
		const char* lpath = vc.path;
		if (vc.path[0] != '/' && vc.dfd != -100)
		{
			__simple_snprintf(buf, sizeof(buf), "/proc/self/fd/%d/%s", vc.dfd, vc.path);
			lpath = buf;
		}
		return fsctl_cas_bsdflags(lpath, data);
	}
	return fsctl_apfs_request(cmd, data, vc.dfd, vc.path, (options & FSOPT_NOFOLLOW) ? LINUX_AT_SYMLINK_NOFOLLOW : 0);
}

long sys_ffsctl(int fd, unsigned long cmd, void* data, unsigned int options)
{
	// Unlike fsctl(), xnu's ffsctl() does answer FSIOC_FD_ONLY_OPEN_ONCE (whether the descriptor is the only open of the
	// file); here it is a file system command like the others and fails with ENOTTY.
	long error = sys_fcntl(fd, F_GETFD_COMMAND, 0);
	if (error < 0)
		return error;
	if ((cmd & 0xffffffffUL) == FSIOC_CAS_BSDFLAGS)
	{
		char buf[32];
		return fsctl_cas_bsdflags(darling_bsdflags_fd_path(fd, buf), data);
	}
	return fsctl_apfs_request(cmd, data, fd, "", FSCTL_AT_EMPTY_PATH);
}
