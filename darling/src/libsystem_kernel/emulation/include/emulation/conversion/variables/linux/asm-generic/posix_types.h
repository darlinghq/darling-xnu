#ifndef DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_POSIX_TYPES_H
#define DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_POSIX_TYPES_H

#include <emulation/conversion/variables/linux/asm/bitsperlong.h>

#ifndef linux___kernel_long_t
typedef long linux___kernel_long_t;
typedef unsigned long linux___kernel_ulong_t;
#endif

#ifndef linux___kernel_pid_t
typedef int linux___kernel_pid_t;
#endif

#ifndef linux___kernel_uid32_t
typedef unsigned int linux___kernel_uid32_t;
typedef unsigned int linux___kernel_gid32_t;
#endif

#ifndef linux___kernel_size_t
#if LINUX___BITS_PER_LONG != 64
typedef unsigned int linux___kernel_size_t;
typedef int linux___kernel_ssize_t;
typedef int linux___kernel_ptrdiff_t;
#else
typedef linux___kernel_ulong_t  linux___kernel_size_t;
typedef linux___kernel_long_t   linux___kernel_ssize_t;
typedef linux___kernel_long_t   linux___kernel_ptrdiff_t;
#endif
#endif // linux___kernel_size_t

#ifndef linux___kernel_fsid_t
typedef struct {
	int	val[2];
} linux___kernel_fsid_t;
#endif

typedef linux___kernel_long_t   linux___kernel_off_t;
typedef long long               linux___kernel_loff_t;
typedef linux___kernel_long_t   linux___kernel_old_time_t;
typedef linux___kernel_long_t   linux___kernel_time_t;
typedef long long               linux___kernel_time64_t;
typedef linux___kernel_long_t   linux___kernel_clock_t;
typedef int                     linux___kernel_timer_t;
typedef int                     linux___kernel_clockid_t;
typedef char *                  linux___kernel_caddr_t;
typedef unsigned short          linux___kernel_uid16_t;
typedef unsigned short          linux___kernel_gid16_t;

#endif // DARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_POSIX_TYPES_HDARLING_EMULATION_CONVERSION_LINUX_ASM_GENERIC_POSIX_TYPES_H
