#ifndef DARLING_EMULATION_CONVERSION_LINUX_BITS_SOCKET_H
#define DARLING_EMULATION_CONVERSION_LINUX_BITS_SOCKET_H

#ifndef DARLING_EMULATION_CONVERSION_LINUX_SYS_SOCKET_H
#error "Use <emulation/conversion/variables/linux/sys/socket.h> instead"
#endif

#include <emulation/conversion/variables/linux/sys/types.h>

// Adding this in for the linux_iovec
#include <emulation/conversion/variables/linux/linux/uio.h>

typedef linux___socklen_t linux_socklen_t;

#define LINUX_PF_LOCAL	1
#define LINUX_PF_INET	2
// LINUX_PF_UNIX
// LINUX_PF_FILE
// LINUX_PF_AX25
#define LINUX_PF_IPX	4
// LINUX_PF_APPLETALK
// LINUX_PF_NETROM
// LINUX_PF_BRIDGE
// LINUX_PF_ATMPVC
// LINUX_PF_X25
#define LINUX_PF_INET6	10

enum
{
    LINUX_MSG_OOB             = 0x01,
    LINUX_MSG_PEEK            = 0x02,
    LINUX_MSG_DONTROUTE       = 0x04,
    LINUX_MSG_TRYHARD         = LINUX_MSG_DONTROUTE,
    LINUX_MSG_CTRUNC          = 0x08,
    LINUX_MSG_PROXY           = 0x10,
    LINUX_MSG_TRUNC           = 0x20,
    LINUX_MSG_DONTWAIT        = 0x40,
    LINUX_MSG_EOR             = 0x80,
    LINUX_MSG_WAITALL         = 0x100,
    LINUX_MSG_FIN             = 0x200,
    LINUX_MSG_SYN             = 0x400,
    LINUX_MSG_CONFIRM         = 0x800,
    LINUX_MSG_RST             = 0x1000,
    LINUX_MSG_ERRQUEUE        = 0x2000,
    LINUX_MSG_NOSIGNAL        = 0x4000,
    LINUX_MSG_MORE            = 0x8000,
    LINUX_MSG_WAITFORONE      = 0x10000,
    LINUX_MSG_BATCH           = 0x40000,
    LINUX_MSG_ZEROCOPY        = 0x4000000,
    LINUX_MSG_FASTOPEN        = 0x20000000,

    LINUX_MSG_CMSG_CLOEXEC    = 0x40000000
};

struct linux_msghdr
{
    void                *msg_name;
    linux_socklen_t     msg_namelen;

    struct linux_iovec    *msg_iov;
    linux_size_t        msg_iovlen;

    void                *msg_control;
    linux_size_t        msg_controllen;
    int                 msg_flags;
};

struct linux_cmsghdr
{
    linux_size_t cmsg_len;
    int cmsg_level;
    int cmsg_type;
    unsigned char cmsg_data[];
};

#define LINUX_CMSG_ALIGN(len) (((len) + sizeof(linux_size_t) - 1) & (linux_size_t) ~(sizeof(linux_size_t) - 1))
#define LINUX_CMSG_SPACE(len) (LINUX_CMSG_ALIGN(len) + LINUX_CMSG_ALIGN(sizeof(struct linux_cmsghdr)))
#define LINUX_CMSG_LEN(len)   (LINUX_CMSG_ALIGN(sizeof (struct linux_cmsghdr)) + (len))

#endif // DARLING_EMULATION_CONVERSION_LINUX_BITS_SOCKET_H