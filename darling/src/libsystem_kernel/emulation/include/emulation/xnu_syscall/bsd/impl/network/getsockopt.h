#ifndef DARLING_BSD_EMULATION_GETSOCKOPT_H
#define DARLING_BSD_EMULATION_GETSOCKOPT_H

long sys_getsockopt(int fd, int level, int optname, void* optval, int* optlen);

#endif // DARLING_BSD_EMULATION_GETSOCKOPT_H
