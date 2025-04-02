#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_STATFS_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_STATFS_H

#include <emulation/conversion/variables/linux/linux/types.h>


#ifndef linux___statfs_word
#if LINUX___BITS_PER_LONG == 64
#define linux___statfs_word linux___kernel_long_t
#else
#define linux___statfs_word linux___u32
#endif
#endif


#ifndef ARCH_PACK_STATFS64
#define ARCH_PACK_STATFS64
#endif

struct linux_statfs64 {
	linux___statfs_word f_type;
	linux___statfs_word f_bsize;
	linux___u64 f_blocks;
	linux___u64 f_bfree;
	linux___u64 f_bavail;
	linux___u64 f_files;
	linux___u64 f_ffree;
	linux___kernel_fsid_t f_fsid;
	linux___statfs_word f_namelen;
	linux___statfs_word f_frsize;
	linux___statfs_word f_flags;
	linux___statfs_word f_spare[4];
} ARCH_PACK_STATFS64;

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_STATFS_H
