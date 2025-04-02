#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_UCRED_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_UCRED_H

#include <emulation/conversion/variables/bsd/sys/param.h>

struct bsd_xucred {
    bsd_u_int   cr_version;
    bsd_uid_t   cr_uid;
    short       cr_ngroups;
    bsd_gid_t   cr_groups[BSD_NGROUPS];
};

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_UCRED_H
