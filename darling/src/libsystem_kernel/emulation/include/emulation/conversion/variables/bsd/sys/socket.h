#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS_SOCKET_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS_SOCKET_H

#include <emulation/conversion/variables/bsd/sys/types.h>
#include <emulation/conversion/variables/bsd/machine/_param.h>

#include <emulation/conversion/variables/bsd/sys/_types/_sa_family_t.h>
#include <emulation/conversion/variables/bsd/sys/_types/_socklen_t.h>

#include <emulation/conversion/variables/bsd/sys/_types/_iovec_t.h>

#define BSD_SO_DEBUG                0x0001
#define BSD_SO_ACCEPTCONN           0x0002
#define BSD_SO_REUSEADDR            0x0004
#define BSD_SO_KEEPALIVE            0x0008
#define BSD_SO_DONTROUTE            0x0010
#define BSD_SO_BROADCAST            0x0020
#define BSD_SO_USELOOPBACK          0x0040
#define BSD_SO_LINGER               0x0080
#define BSD_SO_LINGER_SEC           0x1080

#define BSD_SO_OOBINLINE            0x0100
#define BSD_SO_REUSEPORT            0x0200
#define BSD_SO_TIMESTAMP            0x0400
#define BSD_SO_TIMESTAMP_MONOTONIC  0x0800
#define BSD_SO_DONTTRUNC            0x2000
#define BSD_SO_WANTMORE             0x4000
#define BSD_SO_WANTOOBFLAG          0x8000

#define BSD_SO_SNDBUF               0x1001
#define BSD_SO_RCVBUF               0x1002
#define BSD_SO_SNDLOWAT             0x1003
#define BSD_SO_RCVLOWAT             0x1004
#define BSD_SO_SNDTIMEO             0x1005
#define BSD_SO_RCVTIMEO             0x1006
#define BSD_SO_ERROR                0x1007
#define BSD_SO_TYPE                 0x1008
#define BSD_SO_LABEL                0x1010
#define BSD_SO_PEERLABEL            0x1011
#define BSD_SO_NREAD                0x1020
#define BSD_SO_NKE                  0x1021
#define BSD_SO_NOSIGPIPE            0x1022
#define BSD_SO_NOADDRERR            0x1023
#define BSD_SO_NWRITE               0x1024
#define BSD_SO_REUSESHAREUID        0x1025
#define BSD_SO_NOTIFYCONFLICT       0x1026
#define BSD_SO_UPCALLCLOSEWAIT      0x1027
#define BSD_SO_RANDOMPORT           0x1082
#define BSD_SO_NP_EXTENSIONS        0x1083


struct bsd_linger
{
	int l_onoff;
	int l_linger;
};


#define BSD_SOL_SOCKET          0xffff


#define BSD_AF_UNSPEC           0
#define BSD_AF_UNIX             1
#define BSD_AF_LOCAL            BSD_AF_UNIX
#define BSD_AF_INET             2
#define BSD_AF_IMPLINK          3
#define BSD_AF_PUP              4
#define BSD_AF_CHAOS            5
#define BSD_AF_NS               6
#define BSD_AF_ISO              7
#define BSD_AF_OSI              BSD_AF_ISO
#define BSD_AF_ECMA             8
#define BSD_AF_DATAKIT          9
#define BSD_AF_CCITT            10
#define BSD_AF_SNA              11
#define BSD_AF_DECnet           12
#define BSD_AF_DLI              13
#define BSD_AF_LAT              14
#define BSD_AF_HYLINK           15
#define BSD_AF_APPLETALK        16
#define BSD_AF_ROUTE            17
#define BSD_AF_LINK             18
#define BSD_pseudo_AF_XTP       19
#define BSD_AF_COIP             20
#define BSD_AF_CNT              21
#define BSD_pseudo_AF_RTIP      22
#define BSD_AF_IPX              23
#define BSD_AF_SIP              24
#define BSD_pseudo_AF_PIP       25
#define BSD_AF_NDRV             27
#define BSD_AF_ISDN             28
#define BSD_AF_E164             BSD_AF_ISDN         
#define BSD_pseudo_AF_KEY       29
#define BSD_AF_INET6            30
#define BSD_AF_NATM             31
#define BSD_AF_SYSTEM           32
#define BSD_AF_NETBIOS          33
#define BSD_AF_PPP              34
#define BSD_pseudo_AF_HDRCMPLT  35
#define BSD_AF_RESERVED_36      36 // BSD_AF_AFP
#define BSD_AF_IEEE80211        37
#define BSD_AF_UTUN             38
#define BSD_AF_MULTIPATH        39
#define BSD_AF_VSOCK            40
#define BSD_AF_MAX              41

struct bsd_sockaddr {
    bsd___uint8_t       sa_len;
    bsd_sa_family_t     sa_family;
    char                sa_data[14];
};

#define BSD_PF_UNSPEC       BSD_AF_UNSPEC
#define BSD_PF_LOCAL        BSD_AF_LOCAL
#define BSD_PF_UNIX         BSD_PF_LOCAL
#define BSD_PF_INET         BSD_AF_INET
#define BSD_PF_IMPLINK      BSD_AF_IMPLINK
#define BSD_PF_PUP          BSD_AF_PUP
#define BSD_PF_CHAOS        BSD_AF_CHAOS
#define BSD_PF_NS           BSD_AF_NS
#define BSD_PF_ISO          BSD_AF_ISO
#define BSD_PF_OSI          BSD_AF_ISO
#define BSD_PF_ECMA         BSD_AF_ECMA
#define BSD_PF_DATAKIT      BSD_AF_DATAKIT
#define BSD_PF_CCITT        BSD_AF_CCITT
#define BSD_PF_SNA          BSD_AF_SNA
#define BSD_PF_DECnet       BSD_AF_DECnet
#define BSD_PF_DLI          BSD_AF_DLI
#define BSD_PF_LAT          BSD_AF_LAT
#define BSD_PF_HYLINK       BSD_AF_HYLINK
#define BSD_PF_APPLETALK    BSD_AF_APPLETALK
#define BSD_PF_ROUTE        BSD_AF_ROUTE
#define BSD_PF_LINK         BSD_AF_LINK
#define BSD_PF_XTP          BSD_pseudo_AF_XTP
#define BSD_PF_COIP         BSD_AF_COIP
#define BSD_PF_CNT          BSD_AF_CNT
#define BSD_PF_SIP          BSD_AF_SIP
#define BSD_PF_IPX          BSD_AF_IPX
#define BSD_PF_RTIP         BSD_pseudo_AF_RTIP
#define BSD_PF_PIP          BSD_pseudo_AF_PIP
#define BSD_PF_NDRV         BSD_AF_NDRV
#define BSD_PF_ISDN         BSD_AF_ISDN
#define BSD_PF_KEY          BSD_pseudo_AF_KEY
#define BSD_PF_INET6        BSD_AF_INET6
#define BSD_PF_NATM         BSD_AF_NATM
#define BSD_PF_SYSTEM       BSD_AF_SYSTEM
#define BSD_PF_NETBIOS      BSD_AF_NETBIOS
#define BSD_PF_PPP          BSD_AF_PPP
#define BSD_PF_RESERVED_36  BSD_AF_RESERVED_36 // BSD_AF_AFP
#define BSD_PF_UTUN         BSD_AF_UTUN
#define BSD_PF_MULTIPATH    BSD_AF_MULTIPATH
#define BSD_PF_VSOCK        BSD_AF_VSOCK
#define BSD_PF_MAX          BSD_AF_MAX

struct bsd_msghdr {
	void                *msg_name;
	bsd_socklen_t       msg_namelen;
	struct bsd_iovec    *msg_iov;
	int                 msg_iovlen;
	void                *msg_control;
	bsd_socklen_t       msg_controllen;
	int                 msg_flags;
};

#define BSD_MSG_OOB         0x1
#define BSD_MSG_PEEK        0x2
#define BSD_MSG_DONTROUTE   0x4
#define BSD_MSG_EOR         0x8
#define BSD_MSG_TRUNC       0x10
#define BSD_MSG_CTRUNC      0x20
#define BSD_MSG_WAITALL     0x40
#define BSD_MSG_DONTWAIT    0x80
#define BSD_MSG_EOF         0x100
#define BSD_MSG_WAITSTREAM  0x200
#define BSD_MSG_FLUSH       0x400
#define BSD_MSG_HOLD        0x800
#define BSD_MSG_SEND        0x1000
#define BSD_MSG_HAVEMORE    0x2000
#define BSD_MSG_RCVMORE     0x4000
// BSD_MSG_COMPAT
#define BSD_MSG_NEEDSA      0x10000

struct bsd_cmsghdr {
	bsd_socklen_t       cmsg_len;
	int                 cmsg_level;
	int                 cmsg_type;
    
    // Apple doesn't include cmsg_data by default,
    // but we will include it anyway
    unsigned char       cmsg_data[];
};

#define BSD_CMSG_SPACE(l)   (BSD___DARWIN_ALIGN32(sizeof(struct bsd_cmsghdr)) + BSD___DARWIN_ALIGN32(l))
#define BSD_CMSG_LEN(l)     (BSD___DARWIN_ALIGN32(sizeof(struct bsd_cmsghdr)) + (l))
#define BSD_CMSG_ALIGN(n)   ((typeof(n))BSD___DARWIN_ALIGN32(n))

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS_SOCKET_H