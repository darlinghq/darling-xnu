#include <darling/emulation/xnu_syscall/bsd/helper/misc/fstore_passthrough.h>

#include <sys/errno.h>
#include <sys/fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

// Linux UAPI constants. The emulation is compiled against the target's
// headers, so host kernel header values cannot be used here.
#define LINUX_FALLOC_FL_KEEP_SIZE 0x01
#define LINUX_FALLOC_FL_PUNCH_HOLE 0x02
#define LINUX_POSIX_FADV_NORMAL 0
#define LINUX_POSIX_FADV_SEQUENTIAL 2
#define LINUX_POSIX_FADV_WILLNEED 3

// _IOWR('f', 11, struct fiemap) from <linux/fs.h>.
#define LINUX_FS_IOC_FIEMAP 0xc020660bUL

// Enough of Linux's struct fiemap and struct fiemap_extent to ask for one
// extent. The 64-bit fields are split into (low, high) long words on 32-bit
// hosts by LL_ARG for the syscalls below.
struct linux_fiemap_extent {
	unsigned long long fe_logical;
	unsigned long long fe_physical;
	unsigned long long fe_length;
	unsigned int fe_flags;
	unsigned long long fe_reserved64[2];
	unsigned int fe_reserved[3];
};

struct linux_fiemap_one {
	unsigned long long fm_start;
	unsigned long long fm_length;
	unsigned int fm_flags;
	unsigned int fm_mapped_extents;
	unsigned int fm_extent_count;
	unsigned int fm_reserved;
	struct linux_fiemap_extent fm_extents[1];
};

static long host_error(long ret)
{
	long error = errno_linux_to_bsd(ret);

	// Linux reports an unsupported filesystem operation as EOPNOTSUPP;
	// Darwin's name for the same error is ENOTSUP.
	return error == -EOPNOTSUPP ? -ENOTSUP : error;
}

static long preallocate(int fd, fstore_t* store)
{
	long long start;
	long ret;

	if (store == NULL)
		return -EFAULT;
	if (store->fst_flags & ~(F_ALLOCATECONTIG | F_ALLOCATEALL))
		return -EINVAL;
	if (store->fst_length < 0)
		return -EINVAL;
	// nothing to reserve; macOS 26 answers success
	if (store->fst_length == 0)
	{
		store->fst_bytesalloc = 0;
		return 0;
	}

	if (store->fst_posmode == F_PEOFPOSMODE)
	{
		long end = LINUX_SYSCALL(__NR_lseek, fd, 0, 2 /* SEEK_END */);
		if (end < 0)
			return host_error(end);
		start = (long long) end + store->fst_offset;
	}
	else if (store->fst_posmode == F_VOLPOSMODE)
	{
		// There is no guest volume abstraction; the host file offset is
		// the volume-relative offset here.
		start = store->fst_offset;
	}
	else
	{
		return -EINVAL;
	}

	if (start < 0)
		return -EINVAL;

	// Linux fallocate() reserves space without changing the file size with
	// FALLOC_FL_KEEP_SIZE, matching F_PREALLOCATE. It allocates all the
	// requested space or fails, which also satisfies F_ALLOCATEALL.
	ret = LINUX_SYSCALL(__NR_fallocate, fd, LINUX_FALLOC_FL_KEEP_SIZE,
			LL_ARG((unsigned long long) start), LL_ARG((unsigned long long) store->fst_length));
	if (ret < 0)
		return host_error(ret);

	store->fst_bytesalloc = store->fst_length;
	return 0;
}

static long punch_hole(int fd, fpunchhole_t* punch)
{
	long ret;

	if (punch == NULL)
		return -EFAULT;
	if (punch->fp_flags != 0 || punch->reserved != 0)
		return -EINVAL;
	if (punch->fp_offset < 0 || punch->fp_length <= 0)
		return -EINVAL;

	ret = LINUX_SYSCALL(__NR_fallocate, fd,
			LINUX_FALLOC_FL_PUNCH_HOLE | LINUX_FALLOC_FL_KEEP_SIZE,
			LL_ARG((unsigned long long) punch->fp_offset),
			LL_ARG((unsigned long long) punch->fp_length));
	return ret < 0 ? host_error(ret) : 0;
}

static long advise_read(int fd, struct radvisory* advice)
{
	long ret;

	if (advice == NULL)
		return -EFAULT;
	if (advice->ra_offset < 0 || advice->ra_count < 0)
		return -EINVAL;

	ret = LINUX_SYSCALL(__NR_fadvise64, fd,
			LL_ARG((unsigned long long) advice->ra_offset),
			LL_ARG((unsigned long long) advice->ra_count),
			LINUX_POSIX_FADV_WILLNEED);
	return ret < 0 ? host_error(ret) : 0;
}

static long set_readahead(int fd, long enable)
{
	long ret;

	if (enable != 0 && enable != 1)
		return -EINVAL;

	ret = LINUX_SYSCALL(__NR_fadvise64, fd, LL_ARG(0ULL), LL_ARG(0ULL),
			enable ? LINUX_POSIX_FADV_SEQUENTIAL : LINUX_POSIX_FADV_NORMAL);
	return ret < 0 ? host_error(ret) : 0;
}

static long full_sync(int fd, bool data_only)
{
	long ret = LINUX_SYSCALL1(data_only ? __NR_fdatasync : __NR_fsync, fd);

	return ret < 0 ? host_error(ret) : 0;
}

static long logical_to_physical(int fd, unsigned long cmd, void* data)
{
	struct log2phys* mapping = data;
	struct linux_fiemap_one map;
	unsigned long long offset, length;
	long ret;

	if (mapping == NULL)
		return -EFAULT;

	if (cmd == F_LOG2PHYS)
	{
		long current = LINUX_SYSCALL(__NR_lseek, fd, 0, 1 /* SEEK_CUR */);
		if (current < 0)
			return host_error(current);
		offset = (unsigned long long) current;
		length = 1;
	}
	else
	{
		offset = (unsigned long long) mapping->l2p_devoffset;
		length = (unsigned long long) mapping->l2p_contigbytes;
	}

	if (offset > 0x7fffffffffffffffULL || length == 0)
		return -EINVAL;

	memset(&map, 0, sizeof(map));
	map.fm_start = offset;
	map.fm_length = length;
	map.fm_extent_count = 1;

	ret = LINUX_SYSCALL(__NR_ioctl, fd, LINUX_FS_IOC_FIEMAP, &map);
	if (ret < 0)
		return host_error(ret);
	if (map.fm_mapped_extents == 0)
		return -ENXIO;

	if (offset < map.fm_extents[0].fe_logical ||
		offset - map.fm_extents[0].fe_logical >= map.fm_extents[0].fe_length)
		return -ENXIO;

	offset -= map.fm_extents[0].fe_logical;
	mapping->l2p_devoffset = (off_t)(map.fm_extents[0].fe_physical + offset);
	length = map.fm_extents[0].fe_length - offset;
	if (cmd == F_LOG2PHYS_EXT && length > (unsigned long long) mapping->l2p_contigbytes)
		length = (unsigned long long) mapping->l2p_contigbytes;
	mapping->l2p_contigbytes = (off_t) length;
	return 0;
}

long darling_fstore_passthrough(int fd, unsigned long cmd, void* data)
{
	switch (cmd)
	{
		case F_PREALLOCATE:
			return preallocate(fd, data);
		case F_PUNCHHOLE:
			return punch_hole(fd, data);
		case F_RDADVISE:
			return advise_read(fd, data);
		case F_RDAHEAD:
			return set_readahead(fd, (long) data);
		case F_FULLFSYNC:
			return full_sync(fd, false);
		case F_BARRIERFSYNC:
			return full_sync(fd, true);
		case F_LOG2PHYS:
		case F_LOG2PHYS_EXT:
			return logical_to_physical(fd, cmd, data);
		default:
			return -ENOTTY;
	}
}
