#ifndef DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__TIMESPEC_T_H
#define DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__TIMESPEC_T_H

#include <emulation/conversion/variables/bsd/machine/types.h>

struct bsd_timespec
{
	bsd___darwin_time_t tv_sec;
	long                tv_nsec;
};

#endif // DARLING_EMULATION_CONVERSION_BSD_SYS__TYPES__TIMESPEC_T_H