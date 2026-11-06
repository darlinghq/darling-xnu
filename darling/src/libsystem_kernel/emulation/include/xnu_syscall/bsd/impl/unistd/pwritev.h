#ifndef LINUX_PWRITEV_H
#define LINUX_PWRITEV_H

struct iovec;

long sys_pwritev(int fd, struct iovec* iovp, unsigned int len, long long ofs);
long sys_pwritev_nocancel(int fd, struct iovec* iovp, unsigned int len, long long ofs);

#endif // LINUX_PWRITEV_H
