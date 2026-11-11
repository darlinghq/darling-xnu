#ifndef LINUX_NTP_ADJTIME_H
#define LINUX_NTP_ADJTIME_H

#include <sys/timex.h>

long sys_ntp_adjtime(struct timex* tp);

#endif // LINUX_NTP_ADJTIME_H
