#ifndef LINUX_MACH_EVENTLINK_H
#define LINUX_MACH_EVENTLINK_H

#include <stdint.h>

// These return a 64-bit value (the signal count, with the error in the top byte), which a 32-bit process
// receives in %edx:%eax: the return type has to be 64 bits wide there too.

uint64_t sys_mach_eventlink_signal(unsigned int eventlink_port, uint64_t signal_count);
uint64_t sys_mach_eventlink_wait_until(unsigned int eventlink_port, uint64_t wait_count, uint64_t deadline,
	unsigned int clock_id, unsigned int option);
uint64_t sys_mach_eventlink_signal_wait_until(unsigned int eventlink_port, uint64_t wait_count, uint64_t signal_count,
	uint64_t deadline, unsigned int clock_id, unsigned int option);

#endif // LINUX_MACH_EVENTLINK_H
