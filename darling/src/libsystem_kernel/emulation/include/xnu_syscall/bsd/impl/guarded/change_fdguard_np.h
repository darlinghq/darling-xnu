#ifndef LINUX_CHANGE_FDGUARD_NP_H
#define LINUX_CHANGE_FDGUARD_NP_H

#include <stdint.h>

long sys_change_fdguard_np(int fd, const uint64_t* guard, unsigned int guardflags,
	const uint64_t* nguard, unsigned int nguardflags, int* fdflagsp);

#endif // LINUX_CHANGE_FDGUARD_NP_H
