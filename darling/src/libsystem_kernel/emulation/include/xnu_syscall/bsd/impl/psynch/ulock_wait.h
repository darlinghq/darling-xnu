#ifndef LINUX_ULOCK_WAIT_H
#define LINUX_ULOCK_WAIT_H

#include <stdint.h>

long sys_ulock_wait(uint32_t operation, void* addr, uint64_t value, uint32_t timeout);
long sys_ulock_wait2(uint32_t operation, void* addr, uint64_t value, uint64_t timeout, uint64_t value2);

#define XNU_UL_OPCODE_MASK					0x000000ff
#define XNU_UL_COMPARE_AND_WAIT				1
#define XNU_UL_UNFAIR_LOCK					2
#define XNU_UL_COMPARE_AND_WAIT_SHARED		3
#define XNU_UL_UNFAIR_LOCK64_SHARED			4
#define XNU_UL_COMPARE_AND_WAIT64			5
#define XNU_UL_COMPARE_AND_WAIT64_SHARED	6

#define XNU_ULF_WAIT_CANCEL_POINT			0x00020000
#define XNU_ULF_NO_ERRNO					0x01000000

#define FUTEX_WAIT			0
#define FUTEX_PRIVATE_FLAG	128
#define FUTEX_LOCK_PI		6

#endif // LINUX_ULOCK_WAIT_H
