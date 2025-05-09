#ifndef LINUX_ULOCK_WAIT_H
#define LINUX_ULOCK_WAIT_H

#include <stdint.h>

long sys_ulock_wait(uint32_t operation, void* addr, uint64_t value, uint32_t timeout);

#define XNU_UL_OPCODE_MASK		0xff
#define XNU_UL_COMPARE_AND_WAIT	1
#define XNU_UL_UNFAIR_LOCK		2
#define XNU_ULF_NO_ERRNO		0x1000000

#define FUTEX_WAIT			0
#define FUTEX_PRIVATE_FLAG	128
#define FUTEX_LOCK_PI		6

#endif // LINUX_ULOCK_WAIT_H
