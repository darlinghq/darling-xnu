#ifndef DARLING_NTP_GETTIME_H
#define DARLING_NTP_GETTIME_H

struct ntptimeval;

long sys_ntp_gettime(struct ntptimeval* ntvp);

#endif
