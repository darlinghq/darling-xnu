#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_EVENT_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_EVENT_H

#include <stdint.h>

struct bsd_kevent_qos_s {
    uint64_t        ident;
    int16_t         filter;
    uint16_t        flags;
    int32_t         qos;
    uint64_t        udata;
    uint32_t        fflags;
    uint32_t        xflags;
    int64_t         data;
    uint64_t        ext[4];
};

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_EVENT_H
