#ifndef LINUX_FUTIMES_H
#define LINUX_FUTIMES_H

#include <darling/emulation/xnu_syscall/bsd/impl/time/gettimeofday.h>

long sys_futimes(int fd, struct bsd_timeval* tv);

#endif // LINUX_FUTIMES_H
