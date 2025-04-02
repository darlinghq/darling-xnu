#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_MOUNT_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_MOUNT_H

#include <stdint.h>

#include <emulation/conversion/variables/bsd/sys/param.h>

#include <emulation/conversion/variables/bsd/sys/_types/_fsid_t.h>
#include <emulation/conversion/variables/bsd/sys/_types/_uid_t.h>

#define BSD_MFSNAMELEN      15
#define BSD_MFSTYPENAMELEN  16

#define BSD_MNAMELEN_32 90

struct bsd_statfs64 {
    uint32_t    f_bsize;
    int32_t     f_iosize;
    uint64_t    f_blocks;
    uint64_t    f_bfree;
    uint64_t    f_bavail;
    uint64_t    f_files;
    uint64_t    f_ffree;
    bsd_fsid_t  f_fsid;
    bsd_uid_t   f_owner;
    uint32_t    f_type;
    uint32_t    f_flags;
    uint32_t    f_fssubtype;
    char        f_fstypename[BSD_MFSTYPENAMELEN];
    char        f_mntonname[BSD_MAXPATHLEN];
    char        f_mntfromname[BSD_MAXPATHLEN];
    uint32_t    f_flags_ext;
    uint32_t    f_reserved[7];
};

// TODO: We would have to be careful about the size of these values when
// we implement emulation support for platform with different sizes (ex:
// 64bit host to 32bit guest)

struct bsd_statfs {
	short       f_otype;
	short       f_oflags;
	long        f_bsize;
	long        f_iosize;
	long        f_blocks;
	long        f_bfree;
	long        f_bavail;
	long        f_files;
	long        f_ffree;
	bsd_fsid_t  f_fsid;
	bsd_uid_t   f_owner;
	short       f_reserved1;
	short       f_type;
	long        f_flags;
	long        f_reserved2[2];
	char        f_fstypename[BSD_MFSNAMELEN];
	char        f_mntonname[BSD_MNAMELEN_32];
	char        f_mntfromname[BSD_MNAMELEN_32];
	char        f_reserved3;
	long        f_reserved4[4];
};

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_MOUNT_H