#ifndef LINUX_GETTIMEOFDAY_H
#define LINUX_GETTIMEOFDAY_H

#include <darling/emulation/conversion/time/gettimeofday.h>
#include <stdint.h>

struct timezone;

long sys_gettimeofday(struct bsd_timeval* tv, struct timezone* tz, uint64_t* mach_time);

#endif // LINUX_GETTIMEOFDAY_H
