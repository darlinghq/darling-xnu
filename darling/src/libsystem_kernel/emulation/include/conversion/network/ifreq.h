#ifndef DARLING_CONVERSION_NETWORK_IFREQ_H
#define DARLING_CONVERSION_NETWORK_IFREQ_H

#include <net/if.h>

#define LINUX_SIOCGIFCONF 0x8912
#define LINUX_SIOCGIFFLAGS 0x8913
#define LINUX_SIOCGIFHWADDR 0x8927
#define LINUX_SIOCGIFBRDADDR 0x8919
#define LINUX_SIOCGIFNETMASK 0x891b
#define LINUX_IFNAMSIZ 16

#define MAC_LENGTH 6

struct linux_sockaddr {
	unsigned short sa_family;
	char sa_data[14];
};

struct linux_ifmap {
	unsigned long mem_start;
	unsigned long mem_end;
	unsigned short base_addr;
	unsigned char irq;
	unsigned char dma;
	unsigned char port;
};

struct linux_ifreq {
	char lifr_name[LINUX_IFNAMSIZ];
	union {
		struct linux_sockaddr lifr_addr;
		struct linux_sockaddr lifr_dstaddr;
		struct linux_sockaddr lifr_broadaddr;
		struct linux_sockaddr lifr_netmask;
		struct linux_sockaddr lifr_hwaddr;
		short lifr_flags;
		int lifr_ifindex;
		int lifr_metric;
		int lifr_mtu;
		struct linux_ifmap lifr_map;
		char lifr_slave[LINUX_IFNAMSIZ];
		char lifr_newname[LINUX_IFNAMSIZ];
		char* lifr_data;
	};
};

#define LINUX_SIOCGIFMTU 0x8921

static inline unsigned int if_flags_linux_to_bsd(unsigned short flags)
{
    unsigned int result = 0;
    if (flags & 0x0001) result |= IFF_UP;
    if (flags & 0x0002) result |= IFF_BROADCAST;
    if (flags & 0x0008) result |= IFF_LOOPBACK;
    if (flags & 0x0010) result |= IFF_POINTOPOINT;
    if (flags & 0x0040) result |= IFF_RUNNING;
    if (flags & 0x0080) result |= IFF_NOARP;
    if (flags & 0x0100) result |= IFF_PROMISC;
    if (flags & 0x0200) result |= IFF_ALLMULTI;
    if (flags & 0x1000) result |= IFF_MULTICAST;
    return result;
}

#endif
