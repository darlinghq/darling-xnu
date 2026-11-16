#ifndef DARLING_NETWORK_SOCKOPT_EXTRA_H
#define DARLING_NETWORK_SOCKOPT_EXTRA_H

// Socket options XNU answers itself that Linux has no direct counterpart for.
// The values are kept per descriptor and cleared when the descriptor is closed.

#define BSD_SO_TRAFFIC_CLASS 0x1086
#define BSD_SO_RECV_TRAFFIC_CLASS 0x1087
#define BSD_SO_NET_SERVICE_TYPE 0x1116
#define BSD_SO_RECV_ANYIF 0x1104
#define BSD_IP_RECVIF 20
#define BSD_IP_RECVDSTADDR_OPT 7

long sockopt_extra_set_tc(int fd, int optname, const void* optval, int optlen);
long sockopt_extra_get_tc(int fd, int optname, void* optval, int* optlen);

long sockopt_extra_set_flag(int fd, int level, int optname, const void* optval, int optlen);
long sockopt_extra_get_flag(int fd, int level, int optname, void* optval, int* optlen);

// Track the program's own IP_PKTINFO separately from the one enabled for IP_RECVIF/IP_RECVDSTADDR.
void sockopt_extra_note_pktinfo(int fd, int on);
int sockopt_extra_recvif(int fd);
int sockopt_extra_recvdst(int fd);
int sockopt_extra_pktinfo_user(int fd);

void sockopt_extra_closed(int fd);

#endif // DARLING_NETWORK_SOCKOPT_EXTRA_H
