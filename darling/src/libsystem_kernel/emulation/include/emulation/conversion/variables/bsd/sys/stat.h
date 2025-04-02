#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_STAT_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_STAT_H

#include <emulation/conversion/variables/bsd/sys/_types.h>

#include <emulation/conversion/variables/bsd/sys/_types/_timespec.h>

#include <emulation/conversion/variables/bsd/sys/_types/_blkcnt_t.h>
#include <emulation/conversion/variables/bsd/sys/_types/_blksize_t.h>
#include <emulation/conversion/variables/bsd/sys/_types/_dev_t.h>
#include <emulation/conversion/variables/bsd/sys/_types/_ino_t.h>

#include <emulation/conversion/variables/bsd/sys/_types/_mode_t.h>
#include <emulation/conversion/variables/bsd/sys/_types/_nlink_t.h>
#include <emulation/conversion/variables/bsd/sys/_types/_uid_t.h>
#include <emulation/conversion/variables/bsd/sys/_types/_gid_t.h>
#include <emulation/conversion/variables/bsd/sys/_types/_off_t.h>

#if !defined(__i386__) && !defined(__x86_64__)
#error "[Safety Check] Verify Syscall Implementation Of Stat Is Correct"
#endif

// Compared to the official <sys/stat.h> header, we will always provide the
// legacy `stat` and the modern `stat64` struct. When adding support for additional
// architectures, it's important to check that the syscalls use the correct
// `stat` struct. 

struct bsd_stat {
    bsd_dev_t           st_dev;         /* [XSI] ID of device containing file */
    bsd_ino32_t         st_ino;         /* [XSI] File serial number */
    bsd_mode_t          st_mode;        /* [XSI] Mode of file (see below) */
    bsd_nlink_t         st_nlink;       /* [XSI] Number of hard links */
    bsd_uid_t           st_uid;         /* [XSI] User ID of the file */
    bsd_gid_t           st_gid;         /* [XSI] Group ID of the file */
    bsd_dev_t           st_rdev;        /* [XSI] Device ID */
    struct bsd_timespec st_atimespec;  /* time of last access */
    struct bsd_timespec st_mtimespec;  /* time of last data modification */
    struct bsd_timespec st_ctimespec;  /* time of last status change */
    bsd_off_t           st_size;        /* [XSI] file size, in bytes */
    bsd_blkcnt_t        st_blocks;      /* [XSI] blocks allocated for file */
    bsd_blksize_t       st_blksize;     /* [XSI] optimal blocksize for I/O */
    bsd___uint32_t      st_flags;       /* user defined flags for file */
    bsd___uint32_t      st_gen;         /* file generation number */
    bsd___int32_t       st_lspare;      /* RESERVED: DO NOT USE! */
    bsd___int64_t       st_qspare[2];   /* RESERVED: DO NOT USE! */
};

struct bsd_stat64 {
    bsd_dev_t               st_dev;                 /* [XSI] ID of device containing file */
    bsd_mode_t              st_mode;                /* [XSI] Mode of file (see below) */
    bsd_nlink_t             st_nlink;               /* [XSI] Number of hard links */
    bsd___darwin_ino64_t	st_ino;                /* [XSI] File serial number */
    bsd_uid_t				st_uid;                 /* [XSI] User ID of the file */
    bsd_gid_t				st_gid;                 /* [XSI] Group ID of the file */
    bsd_dev_t				st_rdev;                /* [XSI] Device ID */
    struct bsd_timespec     st_atimespec;           /* time of last access */
    struct bsd_timespec     st_mtimespec;           /* time of last data modification */
    struct bsd_timespec     st_ctimespec;           /* time of last status change */
    struct bsd_timespec     st_birthtimespec;       /* time of file creation(birth) */
    bsd_off_t               st_size;                /* [XSI] file size, in bytes */
    bsd_blkcnt_t            st_blocks;              /* [XSI] blocks allocated for file */
    bsd_blksize_t           st_blksize;             /* [XSI] optimal blocksize for I/O */
    bsd___uint32_t          st_flags;               /* user defined flags for file */
    bsd___uint32_t          st_gen;                 /* file generation number */
    bsd___int32_t           st_lspare;              /* RESERVED: DO NOT USE! */
    bsd___int64_t           st_qspare[2];           /* RESERVED: DO NOT USE! */
};

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_STAT_H