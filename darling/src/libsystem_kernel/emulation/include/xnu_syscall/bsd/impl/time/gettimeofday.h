#ifndef LINUX_GETTIMEOFDAY_H
#define LINUX_GETTIMEOFDAY_H

#include <darling/emulation/conversion/time/gettimeofday.h>

struct timezone;

long sys_gettimeofday(struct bsd_timeval* tv, struct timezone* tz);

#endif // LINUX_GETTIMEOFDAY_H
