#include <stddef.h>
#include <sys/errno.h>
#include <sys/stat.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/dirent/getdirentries.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/xnu_syscall/bsd/impl/dirent/getdirentries.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/dup.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define ATTR_BIT_MAP_COUNT 5

#define COMMON_SUPPORTED (ATTR_CMN_NAME | ATTR_CMN_OBJTYPE | ATTR_CMN_FNDRINFO | ATTR_CMN_OBJTAG)
#define VOLUME_SUPPORTED 0
#define DIR_SUPPORTED (ATTR_DIR_ENTRYCOUNT)
#define FILE_SUPPORTED (ATTR_FILE_RSRCLENGTH)
#define FORK_SUPPORTED 0

#define ATTR_CMN_NAME 0x00000001
#define ATTR_CMN_OBJTYPE 0x00000008
#define ATTR_CMN_FNDRINFO 0x4000
#define ATTR_FILE_RSRCLENGTH 0x1000
#define ATTR_CMN_OBJTAG 0x00000010
#define ATTR_DIR_ENTRYCOUNT 0x00000002

// Darwin xattr names are stored under "user." on the host (xattr_utils.h)
#define XATTR_FINDER_INFO "user.com.apple.FinderInfo"
#define XATTR_RESOURCE_FORK "user.com.apple.ResourceFork"

#define FSOPT_NOFOLLOW 1
#define FSOPT_REPORT_FULLSIZE 4

#define VT_HFS 16

#define min(a,b) (((a) < (b)) ? (a) : (b))

extern void *memcpy(void *dest, const void *src, __SIZE_TYPE__ n);
extern void *memset(void *s, int c, __SIZE_TYPE__ n);
extern char *strcpy(char *dest, const char *src);
extern __SIZE_TYPE__ strlen(const char *s);

static uint32_t linux_mode_to_vtype(unsigned int mode)
{
	if (S_ISREG(mode)) return 1; // VREG
	if (S_ISDIR(mode)) return 2; // VDIR
	if (S_ISBLK(mode)) return 3; // VBLK
	if (S_ISCHR(mode)) return 4; // VCHR
	if (S_ISLNK(mode)) return 5; // VLNK
	if (S_ISSOCK(mode)) return 6; // VSOCK
	if (S_ISFIFO(mode)) return 7; // VFIFO
	return 0; // VNON
}

long
FUNC_NAME(int fd,

#if HAS_PATH
const char* path,
#endif

struct xnu_attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options)
{
	int rv;
	char *ourBuffer, *next;
	__SIZE_TYPE__ spaceNeeded = 4; // 4 bytes for the length header
	__SIZE_TYPE__ fixedSize, nameLength = 0, paddedNameLength = 0;
	const char* name = NULL;
	struct linux_stat linuxStat;

	if (!alist)
		return -EFAULT;

#if HAS_PATH
	if (!path)
		return -EFAULT;

	struct vchroot_expand_args vc;
	vc.flags = (options & FSOPT_NOFOLLOW) ? 0 : VCHROOT_FOLLOW;
	vc.dfd = atfd(fd);

	strcpy(vc.path, path);
	rv = vchroot_expand(&vc);
	
	if (rv < 0)
		return rv;
#endif

	if (alist->bitmapcount < ATTR_BIT_MAP_COUNT)
		return -EINVAL;

	if ((alist->commonattr & COMMON_SUPPORTED) != alist->commonattr)
		return -EINVAL;
	if ((alist->volattr & VOLUME_SUPPORTED) != alist->volattr)
		return -EINVAL;
	if ((alist->dirattr & DIR_SUPPORTED) != alist->dirattr)
		return -EINVAL;
	if ((alist->fileattr & FILE_SUPPORTED) != alist->fileattr)
		return -EINVAL;
	if ((alist->forkattr & FORK_SUPPORTED) != alist->forkattr)
		return -EINVAL;

	if (alist->commonattr & ATTR_CMN_NAME) {
#if HAS_PATH
		const char* expanded = vc.path;
		__SIZE_TYPE__ end = strlen(expanded);
		while (end > 1 && expanded[end - 1] == '/') --end;
		name = expanded;
		for (__SIZE_TYPE__ i = 0; i < end; ++i)
			if (expanded[i] == '/' && i + 1 < end) name = expanded + i + 1;
		nameLength = expanded + end - name;
		if (nameLength == 0) { name = "/"; nameLength = 1; }
		paddedNameLength = (nameLength + 1 + 3) & ~(__SIZE_TYPE__)3;
#else
		return -EINVAL;
#endif
		spaceNeeded += 8; // attrreference_t
	}
	// Directory attributes are only returned for directories and file
	// attributes only for other objects; the other group is left out
	// without taking any space. Both need the object's type.
	uint32_t dirattr = 0, fileattr = 0;
	if ((alist->commonattr & ATTR_CMN_OBJTYPE) || alist->dirattr || alist->fileattr) {
#if HAS_PATH
#ifdef __NR_newfstatat
		rv = LINUX_SYSCALL(__NR_newfstatat, vc.dfd, vc.path, &linuxStat,
			(options & FSOPT_NOFOLLOW) ? LINUX_AT_SYMLINK_NOFOLLOW : 0);
#else
		rv = LINUX_SYSCALL(__NR_fstatat64, vc.dfd, vc.path, &linuxStat,
			(options & FSOPT_NOFOLLOW) ? LINUX_AT_SYMLINK_NOFOLLOW : 0);
#endif
#else
		rv = LINUX_SYSCALL(__NR_fstat, fd, &linuxStat);
#endif
		if (rv < 0) return errno_linux_to_bsd(rv);
		if (S_ISDIR(linuxStat.st_mode))
			dirattr = alist->dirattr;
		else
			fileattr = alist->fileattr;
	}
	if (alist->commonattr & ATTR_CMN_OBJTYPE)
		spaceNeeded += 4; // fsobj_type_t

	if (alist->commonattr & ATTR_CMN_FNDRINFO)
		spaceNeeded += 32;
	if (fileattr & ATTR_FILE_RSRCLENGTH)
		spaceNeeded += 8; // off_t
	if (alist->commonattr & ATTR_CMN_OBJTAG)
		spaceNeeded += sizeof(uint32_t); // fsobj_tag_t
	if (dirattr & ATTR_DIR_ENTRYCOUNT)
		spaceNeeded += sizeof(uint32_t);
	fixedSize = spaceNeeded;
	spaceNeeded += paddedNameLength;
	// Only a buffer too small for the length field is an error. A larger
	// buffer that cannot hold everything gets the leading part of the
	// result, and the length field always holds the full size, with or
	// without FSOPT_REPORT_FULLSIZE (as observed on macOS).
	if (!attributeBuffer || bufferSize < 4)
		return -ERANGE;

	ourBuffer = (char*) __builtin_alloca(spaceNeeded);
	memset(ourBuffer, 0, spaceNeeded);
	next = ourBuffer + 4;
	if (alist->commonattr & ATTR_CMN_NAME) {
		int32_t* reference = (int32_t*)next;
		reference[0] = (int32_t)((ourBuffer + fixedSize) - next);
		((uint32_t*)next)[1] = (uint32_t)(nameLength + 1);
		memcpy(ourBuffer + fixedSize, name, nameLength);
		next += 8;
	}
	if (alist->commonattr & ATTR_CMN_OBJTYPE) {
		*((uint32_t*)next) = linux_mode_to_vtype(linuxStat.st_mode);
		next += 4;
	}
	if (alist->commonattr & ATTR_CMN_OBJTAG) {
		// pretend we're always on HFS
		*((uint32_t*)next) = VT_HFS;
		next += 4;
	}

	if (alist->commonattr & ATTR_CMN_FNDRINFO)
	{
#if HAS_PATH
		rv = (options & FSOPT_NOFOLLOW) ?
			LINUX_SYSCALL(__NR_lgetxattr, vc.path, XATTR_FINDER_INFO, next, 32) :
			LINUX_SYSCALL(__NR_getxattr, vc.path, XATTR_FINDER_INFO, next, 32);
#else
		rv = LINUX_SYSCALL(__NR_fgetxattr, fd, XATTR_FINDER_INFO, next, 32);
#endif
		if (rv < 0)
			memset(next, 0, 32);
		next += 32;
	}
	if (dirattr & ATTR_DIR_ENTRYCOUNT) {
		char buf[1024]; // maybe this should be smaller?
		int tmp_fd;

		*((uint32_t*)next) = 0;

#if HAS_PATH
		tmp_fd = LINUX_SYSCALL(__NR_openat, vc.dfd, vc.path, LINUX_O_RDONLY | LINUX_O_DIRECTORY, 0);
		if (tmp_fd < 0) {
			rv = -EIO; // translate all errors into EIO (to avoid confusing our caller)
			goto attr_dir_entrycount_out_no_fd;
		}
#else
		// dupe it to avoid advancing the internal `getdents` position if our caller is using that
		tmp_fd = sys_dup(fd);
		if (tmp_fd < 0) {
			rv = -EIO; // see above
			goto attr_dir_entrycount_out_no_fd;
		}
#endif

		while (1) {
			// there's basically no other way to do this (at least none that either i or the internet know of)
			rv = LINUX_SYSCALL(__NR_getdents64, tmp_fd, buf, sizeof(buf));
			if (rv < 0) {
				rv = -EIO; // same as before: avoid confusing our caller with unexepected errnos
				goto attr_dir_entrycount_out;
			} else if (rv == 0) {
				break;
			}

			for (char* iter = buf; iter < buf + rv; iter += ((struct linux_dirent64*)iter)->d_reclen) {
				++(*((uint32_t*)next));
			}
		}

attr_dir_entrycount_out:
		close_internal(tmp_fd);
attr_dir_entrycount_out_no_fd:
		next += 4;
	}

	if (fileattr & ATTR_FILE_RSRCLENGTH)
	{
#if HAS_PATH
		if (options & FSOPT_NOFOLLOW)
			rv = LINUX_SYSCALL(__NR_lgetxattr, vc.path, XATTR_RESOURCE_FORK, NULL, 0);
		else
			rv = LINUX_SYSCALL(__NR_getxattr, vc.path, XATTR_RESOURCE_FORK, NULL, 0);
#else
		rv = LINUX_SYSCALL(__NR_fgetxattr, fd, XATTR_RESOURCE_FORK, NULL, 0);
#endif
		// File attributes follow directory attributes. Darwin's off_t is
		// eight bytes, but attribute fields may only be four-byte aligned.
		int64_t resourceLength = rv < 0 ? 0 : rv;
		memcpy(next, &resourceLength, sizeof(resourceLength));
		next += sizeof(resourceLength);
	}

	*((uint32_t*) ourBuffer) = spaceNeeded;
	memcpy(attributeBuffer, ourBuffer, min(bufferSize, spaceNeeded));
	
	return 0;
}
