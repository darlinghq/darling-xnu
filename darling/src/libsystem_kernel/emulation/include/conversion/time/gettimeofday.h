#ifndef DARLING_CONVERSION_TIME_GETTIMEOFDAY
#define DARLING_CONVERSION_TIME_GETTIMEOFDAY

#include <stdint.h>

struct bsd_timeval
{
	unsigned long tv_sec;
	int32_t tv_usec;
};

struct linux_timeval
{
	unsigned long tv_sec;
	long tv_usec;
};

#endif // DARLING_CONVERSION_TIME_GETTIMEOFDAY
