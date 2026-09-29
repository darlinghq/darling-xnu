#include <stddef.h>
#include <sys/errno.h>
#include <sys/stat.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/dirent/getdirentries.h>
#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/xnu_syscall/bsd/impl/dirent/getdirentries.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/dup.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define ATTR_BIT_MAP_COUNT 5

#define COMMON_SUPPORTED (ATTR_CMN_FNDRINFO | ATTR_CMN_OBJTAG | 0x00000001 | 0x00000002 | 0x00000004 | \
	0x00000008 | 0x00000200 | 0x00000400 | 0x00000800 | 0x00001000 | \
	0x00008000 | 0x00010000 | 0x00020000 | 0x00040000 | \
	0x00200000 | 0x02000000 | 0x08000000)
#define COMMON_LEGACY_SUPPORTED (ATTR_CMN_FNDRINFO | ATTR_CMN_OBJTAG | ATTR_CMN_NAME | ATTR_CMN_OBJTYPE)
#define ATTR_VOL_CAPABILITIES 0x00020000
#define ATTR_VOL_UUID 0x00040000
#define ATTR_VOL_ATTRIBUTES 0x40000000
#define ATTR_VOL_INFO 0x80000000
#define VOLUME_SUPPORTED (ATTR_VOL_CAPABILITIES | ATTR_VOL_ATTRIBUTES | ATTR_VOL_INFO)
#define DIR_SUPPORTED (ATTR_DIR_ENTRYCOUNT)
#define DIR_SUPPORTED_ALL (DIR_SUPPORTED | 0x00000001 | 0x00000004)
#define FILE_SUPPORTED (ATTR_FILE_RSRCLENGTH)
#define FILE_SUPPORTED_ALL (FILE_SUPPORTED | 0x00000001 | 0x00000200 | \
	0x00000400)
#define FORK_SUPPORTED 0
#define EXTENDED_SUPPORTED (0x00000040)

#define ATTR_CMN_NAME 0x00000001
#define ATTR_CMN_OBJTYPE 0x00000008
#define ATTR_CMN_FNDRINFO 0x4000
#define ATTR_FILE_RSRCLENGTH 0x1000
#define ATTR_CMN_OBJTAG 0x00000010
#define ATTR_CMN_RETURNED_ATTRS 0x80000000
#define ATTR_CMN_ERROR 0x20000000
#define ATTR_DIR_ENTRYCOUNT 0x00000002

#define XATTR_FINDER_INFO "user.com.apple.FinderInfo"
#define XATTR_RESOURCE_FORK "user.com.apple.ResourceFork"

#define FSOPT_NOFOLLOW 1
#define FSOPT_REPORT_FULLSIZE 4
#define FSOPT_PACK_INVAL_ATTRS 0x08
#define FSOPT_ATTR_CMN_EXTENDED 0x20

#define VT_HFS 16

#define min(a,b) (((a) < (b)) ? (a) : (b))

struct attr_width {
	uint32_t bit;
	uint8_t width;
};

static void attribute_pack_support(uint32_t* attributes)
{
	attributes[0] = COMMON_SUPPORTED | ATTR_CMN_RETURNED_ATTRS;
	attributes[1] = VOLUME_SUPPORTED;
	attributes[2] = DIR_SUPPORTED_ALL;
	attributes[3] = FILE_SUPPORTED_ALL;
	attributes[4] = EXTENDED_SUPPORTED;
	// Native claims are limited to direct stat-backed values. Enumeration,
	// path/permission computation, fallback values and generated support
	// structures are implemented by the bridge, not native filesystem fields.
	attributes[5] = attributes[0] & (0x00000002 | ATTR_CMN_OBJTYPE |
		0x00000400 | 0x00000800 | 0x00001000 | 0x00008000 |
		0x00010000 | 0x00020000 | 0x02000000);
	attributes[6] = 0;
	attributes[7] = 0;
	attributes[8] = attributes[3] & (0x00000001 | 0x00000200 | 0x00000400);
	attributes[9] = attributes[4] & 0x00000040;
}

// Fixed Linux statx UAPI layout. Only the result mask and birth time are used;
// retain the complete 256-byte buffer for the kernel's other result fields.
struct attribute_linux_statx {
	uint32_t mask;
	unsigned char before_birth[76];
	int64_t birth_seconds;
	uint32_t birth_nanoseconds;
	uint32_t birth_reserved;
	unsigned char remainder[160];
};
_Static_assert(sizeof(struct attribute_linux_statx) == 256, "Linux statx size");
_Static_assert(offsetof(struct attribute_linux_statx, birth_seconds) == 80, "Linux statx birth offset");

static const struct attr_width common_attr_widths[] = {
	{ 0x00000001, 8 }, { 0x00000002, 4 }, { 0x00000004, 8 },
	{ 0x00000008, 4 }, { ATTR_CMN_OBJTAG, 4 }, { 0x00000020, 8 },
	{ 0x00000040, 8 }, { 0x00000080, 8 }, { 0x00000100, 4 },
	{ 0x00000200, 16 }, { 0x00000400, 16 }, { 0x00000800, 16 },
	{ 0x00001000, 16 }, { 0x00002000, 16 }, { ATTR_CMN_FNDRINFO, 32 },
	{ 0x00008000, 4 }, { 0x00010000, 4 }, { 0x00020000, 4 },
	{ 0x00040000, 4 }, { 0x00080000, 4 }, { 0x00100000, 4 },
	{ 0x00200000, 4 }, { 0x00400000, 8 }, { 0x00800000, 16 },
	{ 0x01000000, 16 }, { 0x02000000, 8 }, { 0x04000000, 8 },
	{ 0x08000000, 8 }, { 0x10000000, 16 }, { 0x20000000, 4 },
	{ 0x40000000, 4 }, { ATTR_CMN_RETURNED_ATTRS, 20 }, { 0, 0 },
};

static const struct attr_width dir_attr_widths[] = {
	{ 0x00000001, 4 }, { ATTR_DIR_ENTRYCOUNT, 4 }, { 0x00000004, 4 },
	{ 0x00000008, 8 }, { 0x00000010, 4 }, { 0x00000020, 8 }, { 0, 0 },
};

static const struct attr_width volume_attr_widths[] = {
	{ 0x00000001, 4 }, { 0x00000002, 4 }, { 0x00000004, 8 },
	{ 0x00000008, 8 }, { 0x00000010, 8 }, { 0x00000020, 8 },
	{ 0x00000040, 8 }, { 0x00000080, 4 }, { 0x00000100, 4 },
	{ 0x00000200, 4 }, { 0x00000400, 4 }, { 0x00000800, 4 },
	{ 0x00001000, 8 }, { 0x00002000, 8 }, { 0x00004000, 4 },
	{ 0x00008000, 8 }, { 0x00010000, 8 }, { ATTR_VOL_CAPABILITIES, 32 },
	{ ATTR_VOL_UUID, 16 }, { 0x10000000, 8 }, { 0x20000000, 8 },
	{ ATTR_VOL_ATTRIBUTES, 40 }, { ATTR_VOL_INFO, 0 }, { 0, 0 },
};

static const struct attr_width file_attr_widths[] = {
	{ 0x00000001, 4 }, { 0x00000002, 8 }, { 0x00000004, 8 },
	{ 0x00000008, 4 }, { 0x00000010, 4 }, { 0x00000020, 4 },
	{ 0x00000200, 8 }, { 0x00000400, 8 }, { ATTR_FILE_RSRCLENGTH, 8 },
	{ 0x00002000, 8 }, { 0, 0 },
};

static const struct attr_width extended_attr_widths[] = {
	{ 0x00000004, 8 }, { 0x00000008, 8 }, { 0x00000010, 8 },
	{ 0x00000020, 8 }, { 0x00000040, 4 }, { 0x00000080, 8 },
	{ 0x00000100, 8 }, { 0x00000200, 8 }, { 0x00000400, 8 }, { 0, 0 },
};

static __SIZE_TYPE__ packed_width(uint32_t mask, const struct attr_width* widths) {
	__SIZE_TYPE__ result = 0;
	for (; widths->bit != 0; ++widths) {
		if (mask & widths->bit)
			result += widths->width;
	}
	return result;
}

extern void *memcpy(void *dest, const void *src, __SIZE_TYPE__ n);
extern void *memset(void *s, int c, __SIZE_TYPE__ n);
extern char *strcpy(char *dest, const char *src);
extern char *strrchr(const char *str, int character);
extern __SIZE_TYPE__ strlen(const char *str);

// Attribute fields are four-byte packed, including eight-byte quantities.
static void pack_uint64(char* destination, uint64_t value)
{
	memcpy(destination, &value, sizeof(value));
}

#if !HAS_PATH
static int attribute_fd_finder_info(int fd, int volume, char* output)
{
	if (volume) {
		// The selected volume root is held with O_PATH, which fgetxattr
		// rejects. Follow its proc-fd reference without reopening the directory
		// for read access or resolving a possibly renamed original path.
		char path[64];
		__simple_sprintf(path, "/proc/self/fd/%d", fd);
		return LINUX_SYSCALL(__NR_getxattr, path, XATTR_FINDER_INFO, output, 32);
	}
	return LINUX_SYSCALL(__NR_fgetxattr, fd, XATTR_FINDER_INFO, output, 32);
}
#endif

static void pack_time(char* destination, int64_t seconds, uint64_t nanoseconds)
{
	memcpy(destination, &seconds, sizeof(seconds));
	memcpy(destination + sizeof(seconds), &nanoseconds, sizeof(nanoseconds));
}

static int count_directory_entries(int fd, const char* path, uint32_t* count) {
	int rv = 0;
	char buf[1024];
	int tmp_fd;

	*count = 0;

	// A duplicate shares the caller's offset; open an independent description.
	tmp_fd = LINUX_SYSCALL(__NR_openat, fd, path, LINUX_O_RDONLY | LINUX_O_DIRECTORY, 0);
	if (tmp_fd < 0)
		return errno_linux_to_bsd(tmp_fd);

	while (1) {
		rv = LINUX_SYSCALL(__NR_getdents64, tmp_fd, buf, sizeof(buf));
		if (rv < 0) {
			rv = errno_linux_to_bsd(rv);
			goto attr_dir_entrycount_out;
		} else if (rv == 0) {
			break;
		}

		for (char* iter = buf; iter < buf + rv; iter += ((struct linux_dirent64*)iter)->d_reclen) {
			struct linux_dirent64* entry = (struct linux_dirent64*)iter;
			if (entry->d_name[0] == '.' && (entry->d_name[1] == '\0' ||
				(entry->d_name[1] == '.' && entry->d_name[2] == '\0')))
				continue;
			if (*count == UINT32_MAX) {
				rv = -EOVERFLOW;
				goto attr_dir_entrycount_out;
			}
			++*count;
		}
	}

attr_dir_entrycount_out:
	close_internal(tmp_fd);
	return rv < 0 ? rv : 0;
}

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

static int attribute_validate_request(struct xnu_attrlist* alist, unsigned long options)
{
	if (alist->bitmapcount != ATTR_BIT_MAP_COUNT)
		return -EINVAL;
	if (alist->volattr & ~0xf007ffffU)
		return -EINVAL;
	if (alist->volattr) {
		// Darwin's volume-common table is narrower than the object table.
		if (alist->commonattr & ~0xa7e7ffffU)
			return -EINVAL;
		if ((alist->commonattr & 0x07c00000U) &&
			!(alist->commonattr & ATTR_CMN_RETURNED_ATTRS))
			return -EINVAL;
	}
	// Darwin routes volume requests separately from per-object groups.
	// Common-extended attributes may accompany them, but legacy forks may not.
	if (alist->volattr && (alist->fileattr || alist->dirattr ||
		(alist->forkattr && !(options & FSOPT_ATTR_CMN_EXTENDED))))
		return -EINVAL;
	if ((alist->dirattr & ~0x3fU) || (alist->fileattr & ~0x37ffU) ||
		(alist->forkattr & ~0x7ffU))
		return -EINVAL;
	if (options & FSOPT_ATTR_CMN_EXTENDED) {
		if (alist->forkattr & 3U)
			return -EINVAL;
	} else if ((alist->forkattr & 0x7fcU) || (alist->commonattr & 0x00180000U)) {
		return -EINVAL;
	}

	int returnSupportedAttributes = (alist->commonattr & ATTR_CMN_RETURNED_ATTRS) != 0;
	int packInvalidAttributes = (options & FSOPT_PACK_INVAL_ATTRS) != 0;
	if (packInvalidAttributes && !returnSupportedAttributes)
		return -EINVAL;

	if (!returnSupportedAttributes && (alist->commonattr & COMMON_LEGACY_SUPPORTED) != alist->commonattr)
		return -ENOTSUP;
	if (!returnSupportedAttributes && (alist->volattr & VOLUME_SUPPORTED) != alist->volattr)
		return -ENOTSUP;
	if (!returnSupportedAttributes && (alist->dirattr & DIR_SUPPORTED) != alist->dirattr)
		return -ENOTSUP;
	if (!returnSupportedAttributes && (alist->fileattr & FILE_SUPPORTED) != alist->fileattr)
		return -ENOTSUP;
	if (!returnSupportedAttributes && (alist->forkattr & FORK_SUPPORTED) != alist->forkattr)
		return -ENOTSUP;

	return 0;
}

static int attribute_directory_mount_status(int fd, const char* path, uint32_t* status);

static long
attribute_get_object(int fd,

#if HAS_PATH
const char* path,
#endif

struct xnu_attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options)
{
	int rv;
	char *ourBuffer, *next;
	__SIZE_TYPE__ spaceNeeded = 4; // 4 bytes for the length header
	int returnSupportedAttributes;
	int packInvalidAttributes;
	struct linux_stat fileStat;
	char* itemName = NULL;
	__SIZE_TYPE__ itemNameLength = 0;
	__SIZE_TYPE__ fullPathLength = 0;
	char fullPath[4096];
	__SIZE_TYPE__ variableSize = 0;
	__SIZE_TYPE__ fixedSize;
	uint32_t packedCommon;
	uint32_t packedDir;
	uint32_t packedFile;
	uint32_t packedExtended;
	uint32_t packedVolume;
	uint32_t directoryCount = 0;
	uint32_t mountStatus = 0;
	uint32_t availableDir = DIR_SUPPORTED_ALL;
	uint32_t availableCommon = COMMON_SUPPORTED;
	struct attribute_linux_statx birthStat = {0};
	uint32_t userAccess = 0;

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

#ifdef __NR_newfstatat
	rv = LINUX_SYSCALL(__NR_newfstatat, vc.dfd, vc.path, &fileStat,
		(options & FSOPT_NOFOLLOW) ? LINUX_AT_SYMLINK_NOFOLLOW : 0);
#else
rv = LINUX_SYSCALL(__NR_fstatat64, vc.dfd, vc.path, &fileStat,
		(options & FSOPT_NOFOLLOW) ? LINUX_AT_SYMLINK_NOFOLLOW : 0);
#endif
	if (rv < 0)
		return errno_linux_to_bsd(rv);
#else
	rv = LINUX_SYSCALL(__NR_fstat, fd, &fileStat);
	if (rv < 0)
		return errno_linux_to_bsd(rv);
#endif

	returnSupportedAttributes = (alist->commonattr & ATTR_CMN_RETURNED_ATTRS) != 0;
	packInvalidAttributes = (options & FSOPT_PACK_INVAL_ATTRS) != 0;
	if (alist->volattr)
		availableCommon &= ~(0x07c00000U | ATTR_CMN_OBJTYPE);

	if ((alist->dirattr & ATTR_DIR_ENTRYCOUNT) && S_ISDIR(fileStat.st_mode)) {
#if HAS_PATH
		rv = count_directory_entries(vc.dfd, vc.path, &directoryCount);
#else
		rv = count_directory_entries(fd, ".", &directoryCount);
#endif
		if (rv < 0)
			return rv;
	}

	if ((alist->dirattr & 4) && S_ISDIR(fileStat.st_mode)) {
#if HAS_PATH
		rv = attribute_directory_mount_status(vc.dfd, vc.path, &mountStatus);
#else
		rv = attribute_directory_mount_status(fd, NULL, &mountStatus);
#endif
		if (rv < 0)
			availableDir &= ~4U;
	}

	if (alist->commonattr & ATTR_CMN_NAME) {
#if HAS_PATH
		__SIZE_TYPE__ end = strlen(vc.path);
		while (end > 1 && vc.path[end - 1] == '/') --end;
		itemName = vc.path;
		for (__SIZE_TYPE__ i = 0; i < end; ++i)
			if (vc.path[i] == '/' && i + 1 < end) itemName = vc.path + i + 1;
		itemNameLength = vc.path + end - itemName + 1;
#else
		if (!returnSupportedAttributes) return -EINVAL;
#endif
	}

	if (alist->commonattr & 0x00000200) {
		// Creation time is not ctime. If statx or the backing filesystem cannot
		// supply it, omit the returned bit (and retain only an invalid slot).
#ifdef __NR_statx
#if HAS_PATH
		rv = LINUX_SYSCALL(__NR_statx, vc.dfd, vc.path,
			(options & FSOPT_NOFOLLOW) ? LINUX_AT_SYMLINK_NOFOLLOW : 0,
			0x0800, &birthStat); // STATX_BTIME
#else
		rv = LINUX_SYSCALL(__NR_statx, fd, "", 0x1000, 0x0800, &birthStat); // AT_EMPTY_PATH
#endif
#else
		rv = -ENOSYS;
#endif
		if (rv < 0 || !(birthStat.mask & 0x0800))
			availableCommon &= ~0x00000200U;
	}

	if (alist->commonattr & 0x00200000) {
		// faccessat's original Linux ABI has no flags argument and cannot
		// test effective credentials. faccessat2 is syscall 439 on our
		// supported Linux ARM64, x86_64 and i386 ABIs.
		for (unsigned mode = 1; mode <= 4; mode <<= 1) {
#if HAS_PATH
			rv = LINUX_SYSCALL(439, vc.dfd, vc.path, mode,
				0x200 | ((options & FSOPT_NOFOLLOW) ? LINUX_AT_SYMLINK_NOFOLLOW : 0)); // AT_EACCESS
#else
			rv = LINUX_SYSCALL(439, fd, "", mode, 0x200 | 0x1000); // AT_EACCESS | AT_EMPTY_PATH
#endif
			if (rv == 0)
				userAccess |= mode;
			else if (rv != -EACCES && rv != -EPERM && rv != -EROFS) {
				// Unknown availability is not equivalent to denied access.
				availableCommon &= ~0x00200000U;
				userAccess = 0;
				break;
			}
		}
	}

	if (alist->commonattr & 0x08000000) {
#if HAS_PATH
		// O_PATH resolves the object without requiring read access or opening
		// devices/FIFOs for I/O. Convert its kernel path into the guest namespace.
		int pathfd = LINUX_SYSCALL(__NR_openat, vc.dfd, vc.path,
			010000000 | LINUX_O_CLOEXEC | ((options & FSOPT_NOFOLLOW) ? LINUX_O_NOFOLLOW : 0), 0);
#else
		int pathfd = fd; // Borrow the caller's descriptor; never close it.
#endif
		rv = pathfd;
		if (pathfd >= 0) {
			struct vchroot_fdpath_args args = { .fd = pathfd, .path = fullPath, .maxlen = sizeof(fullPath) };
			rv = vchroot_fdpath(&args);
#if HAS_PATH
			close_internal(pathfd);
#endif
		}
		if (rv < 0)
			availableCommon &= ~0x08000000U;
		else
			fullPathLength = strlen(fullPath) + 1;
	}

	packedCommon = packInvalidAttributes ? alist->commonattr :
		(alist->commonattr & (availableCommon | ATTR_CMN_RETURNED_ATTRS));
#if !HAS_PATH
	// The fd path does not supply names. Match the returned mask: unsupported
	// fields occupy slots only when the caller explicitly requests placeholders.
	if (!packInvalidAttributes)
		packedCommon &= ~ATTR_CMN_NAME;
#endif
	packedDir = packInvalidAttributes ? alist->dirattr : (alist->dirattr & availableDir);
	packedFile = packInvalidAttributes ? alist->fileattr : (alist->fileattr & FILE_SUPPORTED_ALL);
	// File and directory groups are mutually exclusive for a concrete object,
	// including when unsupported attributes within that group are packed.
	if (S_ISDIR(fileStat.st_mode))
		packedFile = 0;
	else
		packedDir = 0;
	packedExtended = packInvalidAttributes ? alist->forkattr : (alist->forkattr & EXTENDED_SUPPORTED);
	packedVolume = packInvalidAttributes ? alist->volattr : (alist->volattr & VOLUME_SUPPORTED);

	if (packInvalidAttributes || returnSupportedAttributes) {
		spaceNeeded += packed_width(packedCommon, common_attr_widths);
		spaceNeeded += packed_width(packedDir, dir_attr_widths);
		spaceNeeded += packed_width(packedFile, file_attr_widths);
		spaceNeeded += packed_width(packedExtended, extended_attr_widths);
		if (packedCommon & 0x00000001) {
#if HAS_PATH
			variableSize += (itemNameLength + 3) & ~((__SIZE_TYPE__)3);
#endif
		}
		if (packedCommon & 0x08000000) {
			variableSize += (fullPathLength + 3) & ~((__SIZE_TYPE__)3);
		}
		spaceNeeded += variableSize;
	}
	spaceNeeded += packed_width(packedVolume, volume_attr_widths);

	if (!packInvalidAttributes && !returnSupportedAttributes) {
		if (alist->commonattr & ATTR_CMN_NAME) {
			spaceNeeded += 8;
			variableSize = (itemNameLength + 3) & ~((__SIZE_TYPE__)3);
			spaceNeeded += variableSize;
		}
		if (alist->commonattr & ATTR_CMN_OBJTYPE) spaceNeeded += 4;
		if (alist->commonattr & ATTR_CMN_OBJTAG)
			spaceNeeded += sizeof(uint32_t); // fsobj_tag_t
		if (alist->commonattr & ATTR_CMN_FNDRINFO)
			spaceNeeded += 32;
		if (packedDir & ATTR_DIR_ENTRYCOUNT)
			spaceNeeded += sizeof(uint32_t);
		if (packedFile & ATTR_FILE_RSRCLENGTH)
			spaceNeeded += sizeof(int64_t);
	}

	if (!attributeBuffer || bufferSize < 4 ||
		(bufferSize < spaceNeeded && !(options & FSOPT_REPORT_FULLSIZE)))
		return -ERANGE;
	fixedSize = spaceNeeded - variableSize;
	ourBuffer = (char*) __builtin_alloca(spaceNeeded);
	memset(ourBuffer, 0, spaceNeeded);
	next = ourBuffer + 4;
	if (!returnSupportedAttributes && (alist->commonattr & ATTR_CMN_NAME)) {
		((int32_t*)next)[0] = (ourBuffer + fixedSize) - next;
		((uint32_t*)next)[1] = itemNameLength;
		memcpy(ourBuffer + fixedSize, itemName, itemNameLength - 1);
		next += 8;
	}
	if (!returnSupportedAttributes && (alist->commonattr & ATTR_CMN_OBJTYPE)) {
		*((uint32_t*)next) = alist->volattr ? 0 : linux_mode_to_vtype(fileStat.st_mode);
		next += 4;
	}

	if (packInvalidAttributes || returnSupportedAttributes) {
		char* variable = ourBuffer + fixedSize;
		uint32_t* returned = (uint32_t*)next;
		returned[0] = (alist->commonattr & availableCommon) | ATTR_CMN_RETURNED_ATTRS;
#if !HAS_PATH
		returned[0] &= ~ATTR_CMN_NAME;
#endif
		returned[1] = alist->volattr & VOLUME_SUPPORTED;
		returned[2] = packedDir & availableDir;
		returned[3] = packedFile & FILE_SUPPORTED_ALL;
		returned[4] = alist->forkattr & EXTENDED_SUPPORTED;
		next += sizeof(uint32_t) * ATTR_BIT_MAP_COUNT;
		// ERROR is ordered immediately after RETURNED_ATTRS, not by bit value.
		// Successful ordinary records may omit it; PACK_INVAL reserves it.
		if (packedCommon & ATTR_CMN_ERROR) {
			uint32_t error = 0;
			memcpy(next, &error, sizeof(error));
			next += sizeof(error);
			returned[0] |= ATTR_CMN_ERROR;
		}
		for (const struct attr_width* width = common_attr_widths; width->bit != 0; ++width) {
			if (!(packedCommon & width->bit))
				continue;
			if (width->bit == ATTR_CMN_RETURNED_ATTRS || width->bit == ATTR_CMN_ERROR) {
				continue;
			} else if (width->bit == 0x00000001) {
				int32_t* reference = (int32_t*)next;
				reference[0] = variable - next;
				((uint32_t*)next)[1] = itemNameLength;
				if (itemNameLength) memcpy(variable, itemName, itemNameLength - 1);
				variable += (itemNameLength + 3) & ~((__SIZE_TYPE__)3);
			} else if (width->bit == 0x00000002) {
				*((uint32_t*)next) = (uint32_t)fileStat.st_dev;
			} else if (width->bit == 0x00000004) {
				((uint32_t*)next)[0] = (uint32_t)fileStat.st_dev;
				((uint32_t*)next)[1] = (uint32_t)(fileStat.st_dev >> 32);
			} else if (width->bit == 0x00000008) {
				if (!alist->volattr)
					*((uint32_t*)next) = linux_mode_to_vtype(fileStat.st_mode);
			} else if (width->bit == ATTR_CMN_OBJTAG) {
				*((uint32_t*)next) = VT_HFS;
			} else if (width->bit == 0x00000200) {
				if (availableCommon & 0x00000200)
					pack_time(next, birthStat.birth_seconds, birthStat.birth_nanoseconds);
			} else if (width->bit == 0x00000800) {
				pack_time(next, fileStat.st_ctime, fileStat.st_ctime_nsec);
			} else if (width->bit == 0x00000400) {
				pack_time(next, fileStat.st_mtime, fileStat.st_mtime_nsec);
			} else if (width->bit == 0x00001000) {
				pack_time(next, fileStat.st_atime, fileStat.st_atime_nsec);
			} else if (width->bit == ATTR_CMN_FNDRINFO) {
#if HAS_PATH
				rv = (options & FSOPT_NOFOLLOW) ?
					LINUX_SYSCALL(__NR_lgetxattr, vc.path, XATTR_FINDER_INFO, next, 32) :
					LINUX_SYSCALL(__NR_getxattr, vc.path, XATTR_FINDER_INFO, next, 32);
#else
				rv = attribute_fd_finder_info(fd, alist->volattr != 0, next);
#endif
				if (rv < 0)
					memset(next, 0, 32);
			} else if (width->bit == 0x00008000) {
				*((uint32_t*)next) = fileStat.st_uid;
			} else if (width->bit == 0x00010000) {
				*((uint32_t*)next) = fileStat.st_gid;
			} else if (width->bit == 0x00020000) {
				*((uint32_t*)next) = fileStat.st_mode;
			} else if (width->bit == 0x00040000) {
				*((uint32_t*)next) = 0;
			} else if (width->bit == 0x00080000 || width->bit == 0x00100000) {
				// Generation/document IDs need persistent tracking that the
				// bridge does not provide. Zero is an invalid placeholder,
				// not a valid identifier to advertise in RETURNED_ATTRS.
				*((uint32_t*)next) = 0;
			} else if (width->bit == 0x00200000) {
				*((uint32_t*)next) = userAccess;
			} else if (width->bit == 0x02000000) {
				if (availableCommon & 0x02000000)
					pack_uint64(next, fileStat.st_ino);
			} else if (width->bit == 0x08000000) {
				if (fullPathLength) {
					int32_t* reference = (int32_t*)next;
					reference[0] = variable - next;
					((uint32_t*)next)[1] = fullPathLength;
					memcpy(variable, fullPath, fullPathLength);
					variable += (fullPathLength + 3) & ~((__SIZE_TYPE__)3);
				}
			} else if (width->bit == 0x10000000) {
				// Linux stat does not expose arrival time in this directory.
				// Keep PACK_INVAL's zero placeholder; do not advertise ctime
				// as ADDEDTIME (chmod and other metadata updates change it).
			} else if (width->bit == 0x40000000) {
				// No Darwin data-protection class is supplied by this bridge.
				// Reserve only an invalid placeholder, not a supported value.
				*((uint32_t*)next) = 0;
			}
			next += width->width;
		}
		for (const struct attr_width* width = volume_attr_widths; width->bit; ++width) {
			if (!(packedVolume & width->bit))
				continue;
			// The buffer is zeroed: unknown capability bits and unsupported
			// PACK_INVAL slots (including references) stay zero.
			if (width->bit == ATTR_VOL_ATTRIBUTES) {
				attribute_pack_support((uint32_t*)next);
			}
			next += width->width;
		}
		for (const struct attr_width* width = dir_attr_widths; width->bit != 0; ++width) {
			if (!(packedDir & width->bit))
				continue;
			if (width->bit == 0x00000001)
				// Darwin excludes synthetic dot links. Linux st_nlink counts
				// those, not directory hard links; use Darwin's fallback for
				// filesystems without a directory hard-link count.
				*((uint32_t*)next) = 1;
			else if (width->bit == ATTR_DIR_ENTRYCOUNT)
				memcpy(next, &directoryCount, sizeof(directoryCount));
			else if (width->bit == 0x00000004)
				*((uint32_t*)next) = mountStatus;
			next += width->width;
		}
		for (const struct attr_width* width = file_attr_widths; width->bit != 0; ++width) {
			if (!(packedFile & width->bit))
				continue;
			if (width->bit == 0x00000001) {
				*((uint32_t*)next) = fileStat.st_nlink;
			} else if (width->bit == 0x00000200) {
				pack_uint64(next, fileStat.st_size);
			} else if (width->bit == 0x00000400) {
				pack_uint64(next, (uint64_t)fileStat.st_blocks * 512);
			} else if (width->bit == ATTR_FILE_RSRCLENGTH) {
#if HAS_PATH
				rv = (options & FSOPT_NOFOLLOW) ?
					LINUX_SYSCALL(__NR_lgetxattr, vc.path, XATTR_RESOURCE_FORK, NULL, 0) :
					LINUX_SYSCALL(__NR_getxattr, vc.path, XATTR_RESOURCE_FORK, NULL, 0);
#else
				rv = LINUX_SYSCALL(__NR_fgetxattr, fd, XATTR_RESOURCE_FORK, NULL, 0);
#endif
				int64_t length = rv < 0 ? 0 : rv;
				memcpy(next, &length, sizeof(length));
			} else if (width->bit == 0x00002000) {
				// Linux exposes xattr length, not its physical allocation.
				// Zero is an invalid placeholder, not measured storage usage.
				pack_uint64(next, 0);
			}
			next += width->width;
		}
		for (const struct attr_width* width = extended_attr_widths; width->bit != 0; ++width) {
			if (!(packedExtended & width->bit))
				continue;
			if (width->bit == 0x00000040)
				*((uint32_t*)next) = (uint32_t)fileStat.st_dev;
			else if (width->bit == 0x00000200)
				// Clone sharing, sync-root and purgeability flags are not
				// queried. Keep only PACK_INVAL's unknown-field placeholder.
				pack_uint64(next, 0);
			next += width->width;
		}
		goto attributes_packed;
	}

	if (returnSupportedAttributes) {
		uint32_t* returned = (uint32_t*)next;
		returned[0] = (alist->commonattr & COMMON_SUPPORTED) | ATTR_CMN_RETURNED_ATTRS;
		returned[1] = alist->volattr & VOLUME_SUPPORTED;
		returned[2] = alist->dirattr & DIR_SUPPORTED;
		returned[3] = alist->fileattr & FILE_SUPPORTED;
		returned[4] = alist->forkattr & FORK_SUPPORTED;
		next += sizeof(uint32_t) * ATTR_BIT_MAP_COUNT;
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
		rv = attribute_fd_finder_info(fd, alist->volattr != 0, next);
#endif
		if (rv < 0)
			memset(next, 0, 32);
		next += 32;
	}
	if (alist->volattr & ATTR_VOL_CAPABILITIES) {
		uint32_t* capabilities = (uint32_t*)next;
		capabilities[0] = capabilities[1] = capabilities[2] = capabilities[3] = 0;
		capabilities[4] = capabilities[5] = capabilities[6] = capabilities[7] = 0;
		next += sizeof(uint32_t) * 8;
	}
	if (alist->volattr & ATTR_VOL_ATTRIBUTES) {
		attribute_pack_support((uint32_t*)next);
		next += sizeof(uint32_t) * ATTR_BIT_MAP_COUNT * 2;
	}
	if (packedDir & ATTR_DIR_ENTRYCOUNT) {
		memcpy(next, &directoryCount, sizeof(directoryCount));
		next += sizeof(directoryCount);
	}

	if (packedFile & ATTR_FILE_RSRCLENGTH)
	{
#if HAS_PATH
		rv = (options & FSOPT_NOFOLLOW) ?
			LINUX_SYSCALL(__NR_lgetxattr, vc.path, XATTR_RESOURCE_FORK, NULL, 0) :
			LINUX_SYSCALL(__NR_getxattr, vc.path, XATTR_RESOURCE_FORK, NULL, 0);
#else
		rv = LINUX_SYSCALL(__NR_fgetxattr, fd, XATTR_RESOURCE_FORK, NULL, 0);
#endif
		int64_t length = rv < 0 ? 0 : rv;
		memcpy(next, &length, sizeof(length));
		next += sizeof(int64_t);
	}

attributes_packed:

	*((uint32_t*)ourBuffer) = (options & FSOPT_REPORT_FULLSIZE) ?
		spaceNeeded : min(bufferSize, spaceNeeded);

	memcpy(attributeBuffer, ourBuffer, min(bufferSize, spaceNeeded));
	
	return 0;
}

extern int strncmp(const char*, const char*, __SIZE_TYPE__);
#include "mountroot.h"

static int attribute_directory_mount_status(int fd, const char* path, uint32_t* status)
{
	int object = fd;
	if (path) {
		object = LINUX_SYSCALL(__NR_openat, fd, path,
			010000000 | LINUX_O_CLOEXEC | LINUX_O_NOFOLLOW, 0);
		if (object < 0) return errno_linux_to_bsd(object);
	}
	struct vchroot_expand_args guest = { .flags = VCHROOT_FOLLOW, .dfd = -100 };
	strcpy(guest.path, "/");
	int result = vchroot_expand(&guest);
	int guest_fd = -1, root_fd = -1;
	if (result >= 0) {
		guest_fd = LINUX_SYSCALL(__NR_openat, guest.dfd, guest.path,
			010000000 | LINUX_O_CLOEXEC, 0);
		result = guest_fd < 0 ? errno_linux_to_bsd(guest_fd) : 0;
	}
	if (result >= 0) {
		char record[16384];
		root_fd = attribute_open_volume_root(object, guest_fd, record, sizeof(record));
		result = root_fd < 0 ? root_fd : 0;
	}
	if (result >= 0) {
		struct linux_stat object_stat, root_stat;
		long rv = LINUX_SYSCALL(__NR_fstat, object, &object_stat);
		if (rv >= 0) rv = LINUX_SYSCALL(__NR_fstat, root_fd, &root_stat);
		if (rv < 0) result = errno_linux_to_bsd(rv);
		else *status = object_stat.st_dev == root_stat.st_dev &&
			object_stat.st_ino == root_stat.st_ino ? 1 : 0;
	}
	if (root_fd >= 0) close_internal(root_fd);
	if (guest_fd >= 0) close_internal(guest_fd);
	if (path) close_internal(object);
	return result;
}

long FUNC_NAME(int fd,
#if HAS_PATH
	const char* path,
#endif
	struct xnu_attrlist* alist, void* buffer, __SIZE_TYPE__ size, unsigned long options)
{
	// Pure volume capability/availability queries need no root-object fields.
	if (!alist)
		return -EFAULT;
	int validation = attribute_validate_request(alist, options);
	if (validation < 0)
		return validation;
	if (!alist->volattr || !(alist->commonattr & COMMON_SUPPORTED & 0x0027fff7U))
		return attribute_get_object(fd,
#if HAS_PATH
			path,
#endif
			alist, buffer, size, options);

	int object = fd, result;
#if HAS_PATH
	if (!path)
		return -EFAULT;
	struct vchroot_expand_args input = { .flags = (options & FSOPT_NOFOLLOW) ? 0 : VCHROOT_FOLLOW, .dfd = atfd(fd) };
	strcpy(input.path, path);
	result = vchroot_expand(&input);
	if (result < 0)
		return result;
	object = LINUX_SYSCALL(__NR_openat, input.dfd, input.path,
		010000000 | LINUX_O_CLOEXEC | ((options & FSOPT_NOFOLLOW) ? LINUX_O_NOFOLLOW : 0), 0);
	if (object < 0)
		return errno_linux_to_bsd(object);
#endif
	struct vchroot_expand_args guest = { .flags = VCHROOT_FOLLOW, .dfd = -100 };
	strcpy(guest.path, "/");
	result = vchroot_expand(&guest);
	int guest_fd = -1, root_fd = -1;
	if (result >= 0) {
		guest_fd = LINUX_SYSCALL(__NR_openat, guest.dfd, guest.path, 010000000 | LINUX_O_CLOEXEC, 0);
		result = guest_fd < 0 ? errno_linux_to_bsd(guest_fd) : 0;
	}
	if (result >= 0) {
		char record[16384];
		root_fd = attribute_open_volume_root(object, guest_fd, record, sizeof(record));
		result = root_fd < 0 ? root_fd : 0;
	}
	if (guest_fd >= 0)
		close_internal(guest_fd);
#if HAS_PATH
	close_internal(object);
#endif
	if (result < 0)
		return result;
	long packed = attribute_get_object(root_fd,
#if HAS_PATH
		".",
#endif
		alist, buffer, size, options);
	close_internal(root_fd);
	return packed;
}
