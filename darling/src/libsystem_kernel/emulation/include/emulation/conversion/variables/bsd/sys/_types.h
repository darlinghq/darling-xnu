#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES_H

#include <emulation/conversion/variables/bsd/sys/machine/_types.h>

typedef bsd___int64_t       bsd___darwin_blkcnt_t;
typedef bsd___int32_t       bsd___darwin_blksize_t;
typedef bsd___int32_t       bsd___darwin_dev_t;
typedef unsigned int        bsd___darwin_fsblkcnt_t;
typedef unsigned int        bsd___darwin_fsfilcnt_t;
typedef bsd___uint32_t      bsd___darwin_gid_t;
typedef bsd___uint32_t      bsd___darwin_id_t;
typedef bsd___uint64_t      bsd___darwin_ino64_t;

// Instead of `__darwin_ino_t`, we will provide an explicit 32bit version.
typedef bsd___uint32_t      bsd___darwin_ino32_t; // #if !__DARWIN_64_BIT_INO_T

typedef bsd___uint16_t      bsd___darwin_mode_t;
typedef bsd___int64_t       bsd___darwin_off_t;
typedef bsd___int32_t       bsd___darwin_pid_t;
typedef bsd___uint32_t      bsd___darwin_sigset_t;
typedef bsd___int32_t       bsd___darwin_suseconds_t;
typedef bsd___uint32_t      bsd___darwin_uid_t;

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES_H