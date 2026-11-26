#ifndef DARLING_MSG_X_H
#define DARLING_MSG_X_H

#include <stddef.h>
#include <darling/emulation/conversion/network/recvmsg.h>

// bsd/sys/socket.h: struct msghdr_x is struct msghdr followed by msg_datalen.
struct bsd_msghdr_x {
	struct bsd_msghdr hdr;
	size_t msg_datalen;
};

long sys_recvmsg_x(int socket, struct bsd_msghdr_x* msgp, unsigned int cnt, int flags);
long sys_sendmsg_x(int socket, struct bsd_msghdr_x* msgp, unsigned int cnt, int flags);

#endif
