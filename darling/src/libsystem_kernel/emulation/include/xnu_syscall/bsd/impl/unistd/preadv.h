#ifndef LINUX_PREADV_H
#define LINUX_PREADV_H

struct iovec;

long sys_preadv(int fd, struct iovec* iovp, unsigned int len, long long ofs);
long sys_preadv_nocancel(int fd, struct iovec* iovp, unsigned int len, long long ofs);

#endif // LINUX_PREADV_H
