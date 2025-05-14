#ifndef LINUX_GETSOCKOPT_H
#define LINUX_GETSOCKOPT_H

long sys_getsockopt(int fd, int level, int optname, void* optval, int* optlen);

#endif // LINUX_GETSOCKOPT_H
