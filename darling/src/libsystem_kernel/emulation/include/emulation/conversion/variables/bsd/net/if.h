#ifndef DARLING_EMULATION_CONVERSION_BSD_NET_IF_H
#define DARLING_EMULATION_CONVERSION_BSD_NET_IF_H

#define BSD_IF_NAMESIZE     16

#include <emulation/conversion/variables/bsd/sys/types.h>
#include <emulation/conversion/variables/bsd/sys/socket.h>

#ifndef BSD_IFNAMSIZ
#define BSD_IFNAMSIZ        BSD_IF_NAMESIZE
#endif

struct bsd_ifdevmtu {
	int     ifdm_current;
	int     ifdm_min;
	int     ifdm_max;
};

#pragma pack(4)
struct bsd_ifkpi {
	unsigned int    ifk_module_id;
	unsigned int    ifk_type;
	union {
		void            *ifk_ptr;
		int             ifk_value;
	} ifk_data;
};
#pragma pack()

#define BSD_IFRTYPE_FUNCTIONAL_UNKNOWN              0
#define BSD_IFRTYPE_FUNCTIONAL_LOOPBACK             1
#define BSD_IFRTYPE_FUNCTIONAL_WIRED                2
#define BSD_IFRTYPE_FUNCTIONAL_WIFI_INFRA           3
#define BSD_IFRTYPE_FUNCTIONAL_WIFI_AWDL            4
#define BSD_IFRTYPE_FUNCTIONAL_CELLULAR             5
#define BSD_IFRTYPE_FUNCTIONAL_INTCOPROC            6
#define BSD_IFRTYPE_FUNCTIONAL_COMPANIONLINK        7
#define BSD_IFRTYPE_FUNCTIONAL_MANAGEMENT           8
#define BSD_IFRTYPE_FUNCTIONAL_LAST                 8

#define bsd_ifr_addr         bsd_ifr_ifru.ifru_addr
#define bsd_ifr_dstaddr      bsd_ifr_ifru.ifru_dstaddr
#define bsd_ifr_broadaddr    bsd_ifr_ifru.ifru_broadaddr
#define bsd_ifr_flags        bsd_ifr_ifru.ifru_flags
#define bsd_ifr_metric       bsd_ifr_ifru.ifru_metric
#define bsd_ifr_mtu          bsd_ifr_ifru.ifru_mtu
#define bsd_ifr_phys         bsd_ifr_ifru.ifru_phys
#define bsd_ifr_media        bsd_ifr_ifru.ifru_media
#define bsd_ifr_data         bsd_ifr_ifru.ifru_data
#define bsd_ifr_devmtu       bsd_ifr_ifru.ifru_devmtu
#define bsd_ifr_intval       bsd_ifr_ifru.ifru_intval
#define bsd_ifr_kpi          bsd_ifr_ifru.ifru_kpi
#define bsd_ifr_wake_flags   bsd_ifr_ifru.ifru_wake_flags
#define bsd_ifr_route_refcnt bsd_ifr_ifru.ifru_route_refcnt
#define bsd_ifr_reqcap       bsd_ifr_ifru.ifru_cap[0]
#define bsd_ifr_curcap       bsd_ifr_ifru.ifru_cap[1]

struct bsd_ifreq {
    char ifr_name[BSD_IFNAMSIZ];
    union {
        struct bsd_sockaddr ifru_addr;
        struct bsd_sockaddr ifru_dstaddr;
        struct bsd_sockaddr ifru_broadaddr;
        short               ifru_flags;
        int                 ifru_metric;
        int                 ifru_mtu;
        int                 ifru_phys;
        int                 ifru_media;
        int                 ifru_intval;
        bsd_caddr_t         ifru_data;
        struct bsd_ifdevmtu ifru_devmtu;
        struct bsd_ifkpi    ifru_kpi;
        bsd_u_int32_t       ifru_wake_flags;
        bsd_u_int32_t       ifru_route_refcnt;
        int                 ifru_cap[2];
        bsd_u_int32_t       ifru_functional_type;
        bsd_u_int32_t       ifru_peer_egress_functional_type;
        bsd_u_int8_t        ifru_is_directlink;
        bsd_u_int8_t        ifru_is_vpn;
    } bsd_ifr_ifru;
};

struct bsd_ifaliasreq {
	char    ifra_name[BSD_IFNAMSIZ];
	struct  bsd_sockaddr ifra_addr;
	struct  bsd_sockaddr ifra_broadaddr;
	struct  bsd_sockaddr ifra_mask;
};

#endif // DARLING_EMULATION_CONVERSION_BSD_NET_IF_H