#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlistbulk.h>
#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlistat.h>
#include <darling/emulation/conversion/dirent/getdirentries.h>
#include <darling/emulation/conversion/xattr/getattrlist.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>

extern int strcmp(const char* lhs, const char* rhs);
extern void* memset(void* destination, int value, __SIZE_TYPE__ size);
extern void* memcpy(void* destination, const void* source, __SIZE_TYPE__ size);

#define ATTR_CMN_NAME 0x00000001
#define ATTR_CMN_RETURNED_ATTRS 0x80000000
#define FSOPT_REPORT_FULLSIZE 4
#define FSOPT_NOFOLLOW 1
#define FSOPT_PACK_INVAL_ATTRS 0x08
#define FSOPT_ATTR_CMN_EXTENDED 0x20
#define FSOPT_LIST_SNAPSHOT 0x40
#define MINIMUM_RECORD_SIZE 32
#define LINUX_SEEK_SET 0
#define LINUX_SEEK_CUR 1

static long bulk_restore_position(int fd, long offset, long count, long error)
{
	long result = LINUX_SYSCALL(__NR_lseek, fd, offset, LINUX_SEEK_SET);
	// A failed rewind loses the retry position; do not report partial success
	// as though the unconsumed entries were still available on the next call.
	if (result < 0)
		return errno_linux_to_bsd(result);
	return count > 0 ? count : error;
}

long sys_getattrlistbulk(int fd, struct xnu_attrlist* alist, void* attribute_buffer,
		__SIZE_TYPE__ buffer_size, unsigned long options)
{
	char entries[4096] __attribute__((aligned(__alignof__(struct linux_dirent64))));
	char* output = attribute_buffer;
	__SIZE_TYPE__ remaining = buffer_size;
	long bytes;
	long count = 0;
	long entry_offset;

	if (alist == NULL || attribute_buffer == NULL)
		return -EFAULT;
	if (alist->bitmapcount != 5)
		return -EINVAL;
	// No snapshot namespace is exposed by this implementation. Do not return
	// ordinary directory entries as though they were filesystem snapshots.
	if (options & FSOPT_LIST_SNAPSHOT)
		return -ENOTSUP;
	// Bulk enumeration always interprets forkattr as common extended bits.
	if ((alist->dirattr & ~0x3fU) || (alist->fileattr & ~0x37ffU) ||
		(alist->forkattr & ~0x7fcU))
		return -EINVAL;
	if ((alist->commonattr & (ATTR_CMN_NAME | ATTR_CMN_RETURNED_ATTRS)) !=
		(ATTR_CMN_NAME | ATTR_CMN_RETURNED_ATTRS) || alist->volattr != 0)
		return -EINVAL;

	entry_offset = LINUX_SYSCALL(__NR_lseek, fd, 0, LINUX_SEEK_CUR);
	if (entry_offset < 0)
		return errno_linux_to_bsd(entry_offset);

read_next_batch:
	bytes = LINUX_SYSCALL(__NR_getdents64, fd, entries, sizeof(entries));
	if (bytes < 0)
		return errno_linux_to_bsd(bytes);
	if (bytes == 0)
		return 0;
	if ((unsigned long)bytes > sizeof(entries))
		return -EIO;

	for (char* cursor = entries; cursor < entries + bytes; ) {
		struct linux_dirent64* entry = (struct linux_dirent64*)cursor;
		uint32_t record_size;
		uint32_t aligned_size;
		long result;

		__SIZE_TYPE__ available = entries + bytes - cursor;
		__SIZE_TYPE__ name_offset = offsetof(struct linux_dirent64, d_name);
		if (available < name_offset + 1)
			return -EIO;
		if (entry->d_reclen < name_offset + 1 || entry->d_reclen > available ||
			entry->d_reclen % __alignof__(struct linux_dirent64) != 0)
			return -EIO;
		__SIZE_TYPE__ name_size = entry->d_reclen - name_offset;
		__SIZE_TYPE__ name_length = 0;
		while (name_length < name_size && entry->d_name[name_length] != 0)
			++name_length;
		if (name_length == 0 || name_length == name_size)
			return -EIO;
		cursor += entry->d_reclen;
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			entry_offset = entry->d_off;
			continue;
		}
		if (remaining < MINIMUM_RECORD_SIZE) {
			return bulk_restore_position(fd, entry_offset, count, -ERANGE);
		}

		result = sys_getattrlistat(fd, entry->d_name, alist, output, remaining,
			options | FSOPT_REPORT_FULLSIZE | FSOPT_NOFOLLOW | FSOPT_ATTR_CMN_EXTENDED);
		if (result == -ERANGE) {
			return bulk_restore_position(fd, entry_offset, count, result);
		}
		if (result < 0) {
			int is_directory = entry->d_type == 4;
			// Placeholder widths depend on object type. DT_UNKNOWN is not a file.
			if (entry->d_type == 0 && (options & FSOPT_PACK_INVAL_ATTRS))
				is_directory = darling_attribute_is_directory(fd, entry->d_name);
			if (is_directory >= 0)
				result = darling_pack_attribute_error(entry->d_name, is_directory,
					alist, output, remaining, options, (uint32_t)-result);
		}
		if (result < 0) {
			// getdents advanced over the entire batch, not just emitted entries.
			// Leave the failing entry available for the next call.
			return bulk_restore_position(fd, entry_offset, count, result);
		}

		memcpy(&record_size, output, sizeof(record_size));
		// Reject corrupt lengths before rounding: a near-UINT32_MAX value
		// otherwise wraps to zero and can count an unadvanced output record.
		if (record_size < sizeof(uint32_t) || record_size > (uint32_t)~7U)
			return -EIO;
		aligned_size = (record_size + 7) & ~7U;
		if (aligned_size > remaining) {
			return bulk_restore_position(fd, entry_offset, count, -ERANGE);
		}
		if (aligned_size > record_size)
			memset(output + record_size, 0, aligned_size - record_size);
		memcpy(output, &aligned_size, sizeof(aligned_size));
		output += aligned_size;
		remaining -= aligned_size;
		++count;
		entry_offset = entry->d_off;
	}

	// A batch containing only dot entries is not end-of-directory. Only a
	// zero-byte getdents result establishes that there are no more entries.
	if (count == 0)
		goto read_next_batch;
	return count;
}
