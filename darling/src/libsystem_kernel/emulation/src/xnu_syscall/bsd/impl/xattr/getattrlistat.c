#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlistat.h>

#define HAS_PATH 1
#define FUNC_NAME sys_getattrlistat

#include <darling/emulation/xnu_syscall/bsd/helper/xattr/getattrlist_generic.c>

// Shared with bulk enumeration; use the same width tables as normal records.
int darling_attribute_is_directory(int fd, const char* name)
{
	struct linux_stat st;
#ifdef __NR_newfstatat
	long result = LINUX_SYSCALL(__NR_newfstatat, fd, name, &st, LINUX_AT_SYMLINK_NOFOLLOW);
#else
	long result = LINUX_SYSCALL(__NR_fstatat64, fd, name, &st, LINUX_AT_SYMLINK_NOFOLLOW);
#endif
	return result < 0 ? errno_linux_to_bsd(result) : !!S_ISDIR(st.st_mode);
}

long darling_pack_attribute_error(const char* name, int is_directory,
	struct xnu_attrlist* alist, void* buffer, __SIZE_TYPE__ available,
	unsigned long options, uint32_t error)
{
	uint32_t common = ATTR_CMN_RETURNED_ATTRS | ATTR_CMN_NAME |
		(alist->commonattr & ATTR_CMN_ERROR);
	__SIZE_TYPE__ fixed = sizeof(uint32_t);
	if (options & FSOPT_PACK_INVAL_ATTRS) {
		fixed += packed_width(alist->commonattr, common_attr_widths);
		fixed += is_directory ? packed_width(alist->dirattr, dir_attr_widths) :
			packed_width(alist->fileattr, file_attr_widths);
		fixed += packed_width(alist->forkattr, extended_attr_widths);
	} else {
		fixed += packed_width(common, common_attr_widths);
	}
	__SIZE_TYPE__ nameLength = strlen(name) + 1;
	__SIZE_TYPE__ total = (fixed + nameLength + 7) & ~((__SIZE_TYPE__)7);
	if (total > available)
		return -ERANGE;
	memset(buffer, 0, total);
	char* bytes = buffer;
	uint32_t length = total;
	memcpy(bytes, &length, sizeof(length));
	memcpy(bytes + 4, &common, sizeof(common));
	char* reference = bytes + 24;
	if (common & ATTR_CMN_ERROR) {
		memcpy(reference, &error, sizeof(error));
		reference += sizeof(error);
	}
	int32_t offset = bytes + fixed - reference;
	uint32_t stringLength = nameLength;
	memcpy(reference, &offset, sizeof(offset));
	memcpy(reference + 4, &stringLength, sizeof(stringLength));
	memcpy(bytes + fixed, name, nameLength);
	return 0;
}
