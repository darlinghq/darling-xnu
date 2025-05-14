#ifndef DARLING_CONVERSION_TIME_SETITIMER
#define DARLING_CONVERSION_TIME_SETITIMER

#include <darling/emulation/conversion/time/gettimeofday.h>

struct bsd_itimerval
{
	struct bsd_timeval it_interval;
	struct bsd_timeval it_value;
};

struct linux_itimerval
{
	struct linux_timeval it_interval;
	struct linux_timeval it_value;
};

#endif // DARLING_CONVERSION_TIME_SETITIMER
