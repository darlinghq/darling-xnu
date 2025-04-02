#ifndef DARLING_EMULATION_CONVERSION_BSD_NETINET_IN_H
#define DARLING_EMULATION_CONVERSION_BSD_NETINET_IN_H

#define BSD_IPPROTO_IP              0
#define BSD_IPPROTO_HOPOPTS         0
#define BSD_IPPROTO_ICMP            1
#define BSD_IPPROTO_IGMP            2
#define BSD_IPPROTO_GGP             3
#define BSD_IPPROTO_IPV4            4
#define BSD_IPPROTO_IPIP            BSD_IPPROTO_IPV4
#define BSD_IPPROTO_TCP             6
#define BSD_IPPROTO_ST              7
#define BSD_IPPROTO_EGP             8
#define BSD_IPPROTO_PIGP            9
#define BSD_IPPROTO_RCCMON          10
#define BSD_IPPROTO_NVPII           11
#define BSD_IPPROTO_PUP             12
#define BSD_IPPROTO_ARGUS           13
#define BSD_IPPROTO_EMCON           14
#define BSD_IPPROTO_XNET            15
#define BSD_IPPROTO_CHAOS           16
#define BSD_IPPROTO_UDP             17
#define BSD_IPPROTO_MUX             18
#define BSD_IPPROTO_MEAS            19
#define BSD_IPPROTO_HMP             20
#define BSD_IPPROTO_PRM             21
#define BSD_IPPROTO_IDP             22
#define BSD_IPPROTO_TRUNK1          23
#define BSD_IPPROTO_TRUNK2          24
#define BSD_IPPROTO_LEAF1           25
#define BSD_IPPROTO_LEAF2           26
#define BSD_IPPROTO_RDP             27
#define BSD_IPPROTO_IRTP            28
#define BSD_IPPROTO_TP              29
#define BSD_IPPROTO_BLT             30
#define BSD_IPPROTO_NSP             31
#define BSD_IPPROTO_INP             32
#define BSD_IPPROTO_SEP             33
#define BSD_IPPROTO_3PC             34
#define BSD_IPPROTO_IDPR            35
#define BSD_IPPROTO_XTP             36
#define BSD_IPPROTO_DDP             37
#define BSD_IPPROTO_CMTP            38
#define BSD_IPPROTO_TPXX            39
#define BSD_IPPROTO_IL              40
#define BSD_IPPROTO_IPV6            41

#define BSD_IP_OPTIONS              1
#define BSD_IP_HDRINCL              2
#define BSD_IP_TOS                  3
#define BSD_IP_TTL                  4
#define BSD_IP_RECVOPTS             5
#define BSD_IP_RECVRETOPTS          6
#define BSD_IP_RECVDSTADDR          7
#define BSD_IP_RETOPTS              8
#define BSD_IP_MULTICAST_IF         9
#define BSD_IP_MULTICAST_TTL        10
#define BSD_IP_MULTICAST_LOOP       11
#define BSD_IP_ADD_MEMBERSHIP       12
#define BSD_IP_DROP_MEMBERSHIP      13
#define BSD_IP_MULTICAST_VIF        14
#define BSD_IP_RSVP_ON              15
#define BSD_IP_RSVP_OFF             16
#define BSD_IP_RSVP_VIF_ON          17
#define BSD_IP_RSVP_VIF_OFF         18
#define BSD_IP_PORTRANGE            19
#define BSD_IP_RECVIF               20
#define BSD_IP_IPSEC_POLICY         21
#define BSD_IP_FAITH                22
#define BSD_IP_STRIPHDR             23
#define BSD_IP_RECVTTL              24


#define BSD___KAME_NETINET_IN_H_INCLUDED_
#include <emulation/conversion/variables/bsd/netinet6/in6.h>
#undef BSD___KAME_NETINET_IN_H_INCLUDED_

#endif // DARLING_EMULATION_CONVERSION_BSD_NETINET_IN_H