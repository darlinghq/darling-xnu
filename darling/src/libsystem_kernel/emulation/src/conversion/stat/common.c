// This is needed so stat is not stat64
// On ARM64, only 64-bit inodes are available, so this is not needed/allowed
#if !defined(__aarch64__) && !defined(__arm64__)
#define _DARWIN_NO_64_BIT_INODE
#endif

// NOTE: in this case, platform-include/sys/stat.h is used
#include <sys/stat.h>

#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/conversion/stat/dev.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getgid.h>

void stat_linux_to_bsd(const struct linux_stat* lstat, struct stat* stat)
{
	stat->st_dev = lstat->st_dev;
	stat->st_mode = lstat->st_mode;
	stat->st_nlink = lstat->st_nlink;
	stat->st_ino = lstat->st_ino;
	stat->st_uid = /*lstat->st_uid*/ sys_getuid();
	stat->st_gid = /*lstat->st_gid*/ sys_getgid();
	if (S_ISCHR(lstat->st_mode) || S_ISBLK(lstat->st_mode))
		stat->st_rdev = dev_linux_to_bsd(lstat->st_rdev);
	else
		stat->st_rdev = lstat->st_rdev;
	stat->st_size = lstat->st_size;
	stat->st_blksize = lstat->st_blksize;
	stat->st_blocks = lstat->st_blocks;
	stat->st_atimespec.tv_sec = lstat->st_atime;
	stat->st_atimespec.tv_nsec = lstat->st_atime_nsec;
	stat->st_mtimespec.tv_sec = lstat->st_mtime;
	stat->st_mtimespec.tv_nsec = lstat->st_mtime_nsec;
	stat->st_ctimespec.tv_sec = lstat->st_ctime;
	stat->st_ctimespec.tv_nsec = lstat->st_ctime_nsec;
	stat->st_flags = 0;
}

void stat_linux_to_bsd64(const struct linux_stat* lstat, darling_stat64_t* stat)
{
	stat->st_dev = lstat->st_dev;
	stat->st_mode = lstat->st_mode;
	stat->st_nlink = lstat->st_nlink;
	stat->st_ino = lstat->st_ino;
	stat->st_uid = /*lstat->st_uid*/ sys_getuid();
	stat->st_gid = /*lstat->st_gid*/ sys_getgid();
	if (S_ISCHR(lstat->st_mode) || S_ISBLK(lstat->st_mode))
		stat->st_rdev = dev_linux_to_bsd(lstat->st_rdev);
	else
		stat->st_rdev = lstat->st_rdev;
	stat->st_size = lstat->st_size;
	stat->st_blksize = lstat->st_blksize;
	stat->st_blocks = lstat->st_blocks;
	stat->st_atimespec.tv_sec = lstat->st_atime;
	stat->st_atimespec.tv_nsec = lstat->st_atime_nsec;
	stat->st_mtimespec.tv_sec = lstat->st_mtime;
	stat->st_mtimespec.tv_nsec = lstat->st_mtime_nsec;
	stat->st_ctimespec.tv_sec = lstat->st_ctime;
	stat->st_ctimespec.tv_nsec = lstat->st_ctime_nsec;
	stat->st_flags = 0;
}

static unsigned int statfs_flags_linux_to_bsd(long linux_flags)
{
	unsigned int bsd_flags = 0;

	#define LINUX_ST_RDONLY      0x0001
	#define LINUX_ST_NOSUID      0x0002
	#define LINUX_ST_NODEV       0x0004
	#define LINUX_ST_NOEXEC      0x0008
	#define LINUX_ST_SYNCHRONOUS 0x0010

	#define BSD_MNT_RDONLY       0x00000001
	#define BSD_MNT_SYNCHRONOUS  0x00000002
	#define BSD_MNT_NOEXEC       0x00000004
	#define BSD_MNT_NOSUID       0x00000008
	#define BSD_MNT_NODEV        0x00000010

	if (linux_flags & LINUX_ST_RDONLY)
		bsd_flags |= BSD_MNT_RDONLY;
	if (linux_flags & LINUX_ST_SYNCHRONOUS)
		bsd_flags |= BSD_MNT_SYNCHRONOUS;
	if (linux_flags & LINUX_ST_NOEXEC)
		bsd_flags |= BSD_MNT_NOEXEC;
	if (linux_flags & LINUX_ST_NOSUID)
		bsd_flags |= BSD_MNT_NOSUID;
	if (linux_flags & LINUX_ST_NODEV)
		bsd_flags |= BSD_MNT_NODEV;

	return bsd_flags;
}

void statfs_linux_to_bsd64(const struct linux_statfs64* lstat, struct bsd_statfs64* stat)
{
	__builtin_memset(stat, 0, sizeof(*stat));
	stat->f_type = lstat->f_type;
	stat->f_bsize = lstat->f_bsize;
	stat->f_iosize = lstat->f_bsize;
	stat->f_blocks = lstat->f_blocks;
	stat->f_bfree = lstat->f_bfree;
	stat->f_bavail = lstat->f_bavail;
	stat->f_files = lstat->f_files;
	stat->f_ffree = lstat->f_ffree;
	stat->f_fsid = lstat->f_fsid;
	stat->f_flags = statfs_flags_linux_to_bsd(lstat->f_flags);
}

// A count that doesn't fit the legacy structure's 32-bit long fields.
//
// QUIRK: macOS reports f_files of 0x7fffffffffffffff (APFS) as -1 (all bits set) in the 32-bit statfs without
// 64-bit inodes. It is not documented how other values above 32 bits are reduced; they are clamped to the same
// all-ones value rather than truncated, so a large volume is never reported as a small one (conservative choice,
// inference only for the range between 2^32 and 2^63).
// Evidence: black-box observation on macOS 10.14.4 i386 (tier 2).
// Test: tests/statfs-legacy-abi.c (check_count_clamp)
static long statfs_legacy_count(unsigned long long value)
{
	if (sizeof(long) == 4 && value > 0xffffffffULL)
		return (long) 0xffffffffUL;
	return (long) value;
}

void statfs_bsd64_to_bsd(const struct bsd_statfs64* wide, struct bsd_statfs* stat)
{
	// f_otype, f_oflags, f_reserved1 and the other fields without a value are 0, as on macOS
	__builtin_memset(stat, 0, sizeof(*stat));
	stat->f_bsize = statfs_legacy_count(wide->f_bsize);
	stat->f_iosize = statfs_legacy_count((unsigned int) wide->f_iosize);
	stat->f_blocks = statfs_legacy_count(wide->f_blocks);
	stat->f_bfree = statfs_legacy_count(wide->f_bfree);
	stat->f_bavail = statfs_legacy_count(wide->f_bavail);
	stat->f_files = statfs_legacy_count(wide->f_files);
	stat->f_ffree = statfs_legacy_count(wide->f_ffree);
	stat->f_fsid = wide->f_fsid;
	stat->f_owner = wide->f_owner;
	stat->f_type = (short) wide->f_type;
	stat->f_flags = (long) wide->f_flags;
	// the names are cut to the legacy field sizes (MFSNAMELEN, MNAMELEN)
	__builtin_memcpy(stat->f_fstypename, wide->f_fstypename, sizeof(stat->f_fstypename) - 1);
	__builtin_memcpy(stat->f_mntonname, wide->f_mntonname, sizeof(stat->f_mntonname) - 1);
	__builtin_memcpy(stat->f_mntfromname, wide->f_mntfromname, sizeof(stat->f_mntfromname) - 1);
}
