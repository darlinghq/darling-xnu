#include <sys/errno.h>
#include <stddef.h>
#include <stdint.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern char *strcpy(char *dest, const char *src);
extern void *memcpy(void *dest, const void *src, __SIZE_TYPE__ n);
extern int do_linux_fchmodat(int fd, const char* path, int mode, int flag);

#define ATTR_BIT_MAP_COUNT 5

#define ATTR_CMN_CRTIME 0x200
#define ATTR_CMN_MODTIME 0x400
#define ATTR_CMN_CHGTIME 0x800
#define ATTR_CMN_ACCTIME 0x1000
#define ATTR_CMN_ACCESSMASK 0x20000
#define ATTR_CMN_EXTENDED_SECURITY 0x400000
#define ATTR_CMN_UUID 0x800000
#define ATTR_CMN_GRPUUID 0x1000000

// Only what is applied is accepted, everything else is EINVAL like an attribute that cannot be set: the access mask and
// the modification and access times, and the file's ACL and owner/group GUIDs as far as there is nothing to set:
// Linux files here have no ACL and no GUIDs of their own, so "no ACL" and "no GUID" are already what the file has
// (see setattrlist_check_security()). The other attributes macOS can set (creation time, change time, backup time,
// Finder info, owner, group, flags) are not applied.
#define COMMON_SUPPORTED (ATTR_CMN_MODTIME|ATTR_CMN_ACCTIME|ATTR_CMN_ACCESSMASK|ATTR_CMN_EXTENDED_SECURITY|ATTR_CMN_UUID|ATTR_CMN_GRPUUID)
#define VOLUME_SUPPORTED 0
#define DIR_SUPPORTED 0
#define FILE_SUPPORTED 0
#define FORK_SUPPORTED 0

#define FSOPT_NOFOLLOW 1

#define LINUX_UTIME_OMIT	((1l << 30) - 2l)

struct timespec
{
	long tv_sec;
	long tv_nsec;
};

// struct kauth_filesec of the ATTR_CMN_EXTENDED_SECURITY value: a magic number, the GUIDs of the owner and the group
// (null: not set), and the ACL: the number of entries (KAUTH_FILESEC_NOACL: none) and flags, then the entries
// of 24 bytes each (a GUID, flags and rights).
#define KAUTH_FILESEC_MAGIC 0x012CC16DU
#define KAUTH_FILESEC_NOACL 0xffffffffU
#define KAUTH_FILESEC_HEADER_SIZE 44
#define KAUTH_ACE_SIZE 24

static int setattrlist_guid_is_null(const unsigned char* guid)
{
	for (int i = 0; i < 16; i++)
		if (guid[i])
			return 0;
	return 1;
}

// Checks an ATTR_CMN_EXTENDED_SECURITY, ATTR_CMN_UUID or ATTR_CMN_GRPUUID value that is still to be set. macOS 26.7.3
// accepts a filesec without an ACL (KAUTH_FILESEC_NOACL or no entries), with entries, and null GUIDs; it refuses with
// EINVAL a wrong magic number, an empty or too short reference, a reference that does not lie in the buffer, and more
// entries than the data holds, and a GUID that is not null with EPERM (not as root). A file here has no ACL and no
// GUIDs, which is what an empty ACL or a null GUID asks for, so those are accepted. ACL entries cannot be kept:
// ENOTSUP (macOS applies them).
static int setattrlist_check_security(const char* value, int is_reference, const char* buffer, __SIZE_TYPE__ bufferSize)
{
	if (!is_reference)
		return setattrlist_guid_is_null((const unsigned char*) value) ? 0 : -EPERM;

	int32_t offset;
	uint32_t length;
	memcpy(&offset, value, 4);
	memcpy(&length, value + 4, 4);
	if (length < KAUTH_FILESEC_HEADER_SIZE)
		return -EINVAL;
	// the reference's offset is from the reference itself
	long start = (value - buffer) + (long) offset;
	if (start < 0 || (unsigned long) start > bufferSize || length > bufferSize - (unsigned long) start)
		return -EINVAL;

	const unsigned char* filesec = (const unsigned char*) buffer + start;
	uint32_t magic, entries;
	memcpy(&magic, filesec, 4);
	memcpy(&entries, filesec + 36, 4);
	if (magic != KAUTH_FILESEC_MAGIC)
		return -EINVAL;
	if (entries != KAUTH_FILESEC_NOACL && entries > (length - KAUTH_FILESEC_HEADER_SIZE) / KAUTH_ACE_SIZE)
		return -EINVAL;
	if ((entries != KAUTH_FILESEC_NOACL && entries != 0) || !setattrlist_guid_is_null(filesec + 4) ||
		!setattrlist_guid_is_null(filesec + 20))
		return -ENOTSUP;
	return 0;
}

long
FUNC_NAME(int fd,

#if HAS_PATH
const char* path,
#endif

struct xnu_attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options)
{
	int rv;
	char* next;
	int linux_at_flag = 0;

	if (options & FSOPT_NOFOLLOW)
		linux_at_flag |= LINUX_AT_SYMLINK_NOFOLLOW;

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

	next = (char*) attributeBuffer;

	// Look at everything that will not be applied before changing anything. The attributes follow each other in the
	// order of their bits; the ones that are applied come first.
	{
		const char* value = next;
		if (alist->commonattr & ATTR_CMN_MODTIME)
			value += sizeof(struct timespec);
		if (alist->commonattr & ATTR_CMN_ACCTIME)
			value += sizeof(struct timespec);
		if (alist->commonattr & ATTR_CMN_ACCESSMASK)
			value += 4;
		__SIZE_TYPE__ size = bufferSize;
		if (alist->commonattr && !attributeBuffer)
			return -EFAULT;
		if ((__SIZE_TYPE__)(value - next) > size)
			return -EINVAL;
		if (alist->commonattr & ATTR_CMN_EXTENDED_SECURITY)
		{
			if ((__SIZE_TYPE__)(value - next) + 8 > size)
				return -EINVAL;
			rv = setattrlist_check_security(value, 1, next, size);
			if (rv < 0)
				return rv;
			value += 8;
		}
		if (alist->commonattr & ATTR_CMN_UUID)
		{
			if ((__SIZE_TYPE__)(value - next) + 16 > size)
				return -EINVAL;
			rv = setattrlist_check_security(value, 0, next, size);
			if (rv < 0)
				return rv;
			value += 16;
		}
		if (alist->commonattr & ATTR_CMN_GRPUUID)
		{
			if ((__SIZE_TYPE__)(value - next) + 16 > size)
				return -EINVAL;
			rv = setattrlist_check_security(value, 0, next, size);
			if (rv < 0)
				return rv;
		}
	}

	if (alist->commonattr & ATTR_CMN_CRTIME)
	{
		struct timespec* ts = (struct timespec*) next;
		next += sizeof(struct timespec);
	}
    if (alist->commonattr & ATTR_CMN_MODTIME)
	{
		struct timespec* ts = (struct timespec*) next;
		struct timespec times[2] = {
			{ LINUX_UTIME_OMIT, LINUX_UTIME_OMIT },
			*ts
		};
#if HAS_PATH
		rv = LINUX_SYSCALL(__NR_utimensat, LINUX_AT_FDCWD, vc.path, times, linux_at_flag);
#else
		rv = LINUX_SYSCALL(__NR_utimensat, fd, 0, times, linux_at_flag);
#endif
		if (rv < 0)
			return errno_linux_to_bsd(rv);
		next += sizeof(struct timespec);
	}
	if (alist->commonattr & ATTR_CMN_CHGTIME)
	{
		struct timespec* ts = (struct timespec*) next;
		next += sizeof(struct timespec);
	}
	if (alist->commonattr & ATTR_CMN_ACCTIME)
	{
		struct timespec* ts = (struct timespec*) next;
		struct timespec times[2] = {
			*ts,
			{ LINUX_UTIME_OMIT, LINUX_UTIME_OMIT }
		};
#if HAS_PATH
		rv = LINUX_SYSCALL(__NR_utimensat, LINUX_AT_FDCWD, vc.path, times, linux_at_flag);
#else
		rv = LINUX_SYSCALL(__NR_utimensat, fd, 0, times, linux_at_flag);
#endif
		if (rv < 0)
			return errno_linux_to_bsd(rv);
		next += sizeof(struct timespec);
	}
    if (alist->commonattr & ATTR_CMN_ACCESSMASK)
	{
		int perms = *((int*) next);
#if HAS_PATH
		rv = do_linux_fchmodat(LINUX_AT_FDCWD, vc.path, perms, linux_at_flag);
#else
		rv = LINUX_SYSCALL(__NR_fchmod, fd, perms);
#endif
		if (rv < 0)
			return errno_linux_to_bsd(rv);
		next += 4;
	}

	return 0;
}
