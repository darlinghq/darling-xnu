#ifndef DARLING_EMULATION_CONVERSION_BSD_NETINET6_IN6_H
#define DARLING_EMULATION_CONVERSION_BSD_NETINET6_IN6_H

#ifndef BSD___KAME_NETINET_IN_H_INCLUDED_
#error "Use <netinet/in.h> instead"
#endif

#include <emulation/conversion/variables/bsd/sys/_types.h>
#include <emulation/conversion/variables/bsd/sys/_types/_sa_family_t.h>

// Not sure were these includes are suppose to belong...
#include <emulation/conversion/variables/bsd/sys/_types/_in_port_t.h>

typedef struct bsd_in6_addr {
	union {
		bsd___uint8_t   __u6_addr8[16];
		bsd___uint16_t  __u6_addr16[8];
		bsd___uint32_t  __u6_addr32[4];
	} bsd___u6_addr;
} bsd_in6_addr_t;

struct bsd_sockaddr_in6 {
	bsd___uint8_t       sin6_len;
	bsd_sa_family_t     sin6_family;
	bsd_in_port_t       sin6_port;
	bsd___uint32_t      sin6_flowinfo;
	struct bsd_in6_addr sin6_addr;
	bsd___uint32_t      sin6_scope_id;
};

#define BSD_IPV6_UNICAST_HOPS       4
#define BSD_IPV6_MULTICAST_IF       9
#define BSD_IPV6_MULTICAST_HOPS     10
#define BSD_IPV6_MULTICAST_LOOP     11
#define BSD_IPV6_JOIN_GROUP         12
#define BSD_IPV6_LEAVE_GROUP        13

#define BSD_IPV6_PORTRANGE          14
#define BSD_ICMP6_FILTER            18
#define BSD_IPV6_2292PKTINFO        19
#define BSD_IPV6_2292HOPLIMIT       20
#define BSD_IPV6_2292NEXTHOP        21
#define BSD_IPV6_2292HOPOPTS        22
#define BSD_IPV6_2292DSTOPTS        23
#define BSD_IPV6_2292RTHDR          24

#define BSD_IPV6_2292PKTOPTIONS     25

#define BSD_IPV6_CHECKSUM           26
#define BSD_IPV6_V6ONLY             27

#endif // DARLING_EMULATION_CONVERSION_BSD_NETINET6_IN6_H
