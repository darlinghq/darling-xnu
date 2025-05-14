#ifndef LINUX_RECVMSG_H
#define LINUX_RECVMSG_H

#include <darling/emulation/conversion/network/recvmsg.h>

long sys_recvmsg(int socket, struct bsd_msghdr* msg, int flags);
long sys_recvmsg_nocancel(int socket, struct bsd_msghdr* msg, int flags);

#endif // LINUX_RECVMSG_H
