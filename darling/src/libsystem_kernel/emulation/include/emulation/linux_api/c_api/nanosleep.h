#ifndef DARLING_EMULATION_LINUX_API_NANOSLEEP_H
#define DARLING_EMULATION_LINUX_API_NANOSLEEP_H

#include <emulation/linux_api/c_api/sys/timerfd.h>

extern int __linux_nanosleep(struct timespec* ts, struct timespec* rem);

#endif // DARLING_EMULATION_LINUX_API_NANOSLEEP_H
