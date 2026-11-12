#ifndef LINUX_MSG_X_H
#define LINUX_MSG_X_H

#include <darling/emulation/conversion/network/recvmsg.h>

// XNU's batched datagram calls (syscalls 480 / 481, libsystem_kernel's recvmsg_x / sendmsg_x). They were
// missing from the BSD table and returned ENOSYS. struct msghdr_x is struct msghdr followed by the datagram's length.
struct bsd_msghdr_x
{
	struct bsd_msghdr hdr;
	size_t msg_datalen;
};

long sys_recvmsg_x(int socket, struct bsd_msghdr_x* msgp, unsigned int cnt, int flags);
long sys_sendmsg_x(int socket, struct bsd_msghdr_x* msgp, unsigned int cnt, int flags);

#endif // LINUX_MSG_X_H
