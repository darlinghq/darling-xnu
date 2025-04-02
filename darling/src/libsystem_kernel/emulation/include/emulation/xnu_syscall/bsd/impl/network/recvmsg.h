#ifndef DARLING_BSD_EMULATION_RECVMSG_H
#define DARLING_BSD_EMULATION_RECVMSG_H

#include <stdint.h>
#include <stddef.h>

#include <emulation/conversion/variables/bsd/sys/socket.h>

// Not sure where I should keep this...
#define LINUX_SYS_RECVMSG	17

long sys_recvmsg(int socket, struct bsd_msghdr* msg, int flags);
long sys_recvmsg_nocancel(int socket, struct bsd_msghdr* msg, int flags);

// TODO: Move into conversion
int socket_level_bsd_to_linux(int level);
int socket_level_linux_to_bsd(int level);

// #define BSD_CMSG_ALIGN(len) (((len) + sizeof(uint32_t) - 1) & (size_t)~(sizeof(uint32_t) - 1))
// #define BSD_CMSG_SPACE(len) (BSD_CMSG_ALIGN(sizeof(struct bsd_cmsghdr)) + BSD_CMSG_ALIGN(len))
// #define BSD_CMSG_LEN(len) (BSD_CMSG_ALIGN(sizeof(struct bsd_cmsghdr)) + (len))

#define LINUX_BSD_CMSGHDR_SIZE_DIFFERENCE (sizeof(struct linux_cmsghdr) - sizeof(struct bsd_cmsghdr))

#endif // DARLING_BSD_EMULATION_RECVMSG_H
