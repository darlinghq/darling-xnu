#ifndef DARLING_EMULATION_CONVERSION_LINUX_SYS_TYPES_H
#define DARLING_EMULATION_CONVERSION_LINUX_SYS_TYPES_H

#include <emulation/conversion/variables/linux/bits/types.h>

#define	__need_size_t
#include <stddef.h>

#define linux_size_t size_t

#endif // DARLING_EMULATION_CONVERSION_LINUX_SYS_TYPES_H