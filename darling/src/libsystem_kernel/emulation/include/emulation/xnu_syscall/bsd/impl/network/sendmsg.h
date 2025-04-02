#ifndef DARLING_BSD_EMULATION_SENDMSG_H
#define DARLING_BSD_EMULATION_SENDMSG_H

#include <stdint.h>

#include <emulation/xnu_syscall/bsd/impl/network/recvmsg.h>

long sys_sendmsg(int socket, const struct bsd_msghdr* msg, int flags);
long sys_sendmsg_nocancel(int socket, const struct bsd_msghdr* msg, int flags);

// TODO: Move into conversion

#define LINUX_SYS_SENDMSG	16

#endif // DARLING_BSD_EMULATION_SENDMSG_H
