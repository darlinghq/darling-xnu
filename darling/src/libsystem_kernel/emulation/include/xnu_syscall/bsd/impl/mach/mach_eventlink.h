#ifndef LINUX_MACH_EVENTLINK_H
#define LINUX_MACH_EVENTLINK_H

#include <stdint.h>

long sys_mach_eventlink_signal(unsigned int eventlink_port, uint64_t signal_count);
long sys_mach_eventlink_wait_until(unsigned int eventlink_port, uint64_t wait_count, uint64_t deadline,
	unsigned int clock_id, unsigned int option);
long sys_mach_eventlink_signal_wait_until(unsigned int eventlink_port, uint64_t wait_count, uint64_t signal_count,
	uint64_t deadline, unsigned int clock_id, unsigned int option);

#endif // LINUX_MACH_EVENTLINK_H
