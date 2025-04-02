#ifndef DARLING_EMULATION_CONVERSION_BSD_NETINET6_IN6_VAR_H
#define DARLING_EMULATION_CONVERSION_BSD_NETINET6_IN6_VAR_H

#include <emulation/conversion/variables/bsd/net/if_var.h>

#include <emulation/conversion/variables/bsd/netinet/in.h>

// Not sure where these includes actually belong in...
#include <emulation/conversion/variables/bsd/sys/filio.h>
#include <emulation/conversion/variables/bsd/sys/_types/_time_t.h>
#include <emulation/conversion/variables/bsd/sys/_types/_u_int32_t.h>

struct bsd_in6_addrlifetime {
	bsd_time_t ia6t_expire;
	bsd_time_t ia6t_preferred;
	bsd_u_int32_t ia6t_vltime;
	bsd_u_int32_t ia6t_pltime;
};

struct bsd_in6_aliasreq {
	char    ifra_name[BSD_IFNAMSIZ];
	struct  bsd_sockaddr_in6 ifra_addr;
	struct  bsd_sockaddr_in6 ifra_dstaddr;
	struct  bsd_sockaddr_in6 ifra_prefixmask;
	int     ifra_flags;
	struct  bsd_in6_addrlifetime ifra_lifetime;
};

// TODO: Duplicate in6_aliasreq struct
#define BSD_SIOCAIFADDR_IN6 BSD__IOW('i', 26, struct bsd_in6_aliasreq)

#endif // DARLING_EMULATION_CONVERSION_BSD_NETINET6_IN6_VAR_H
