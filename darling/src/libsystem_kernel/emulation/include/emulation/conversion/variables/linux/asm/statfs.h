#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_STATFS_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_STATFS_H

#if defined(__x86_64__) || defined(__i386__)
#define LINUX_ARCH_PACK_COMPAT_STATFS64 __attribute__((packed,aligned(4)))

#else
#error "Missing platform specific statfs macros"
#endif 

#include <emulation/conversion/variables/linux/asm-generic/statfs.h>

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_STATFS_H