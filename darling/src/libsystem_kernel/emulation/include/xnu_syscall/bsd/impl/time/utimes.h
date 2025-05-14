#ifndef LINUX_UTIMES_H
#define LINUX_UTIMES_H

#include <darling/emulation/xnu_syscall/bsd/impl/time/gettimeofday.h>

long sys_utimes(const char* path, struct bsd_timeval* tv);

#endif // LINUX_UTIMES_H
