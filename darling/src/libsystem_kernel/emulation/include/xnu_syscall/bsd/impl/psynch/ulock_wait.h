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

// Classification of the operation codes shared by wait and wake.
// The *_SHARED operations work on memory shared between processes; the 64-bit ones
// (COMPARE_AND_WAIT64*) compare the whole 64-bit word at the address.
static inline int xnu_ulock_op_is_shared(uint32_t op)
{
	return op == XNU_UL_COMPARE_AND_WAIT_SHARED || op == XNU_UL_UNFAIR_LOCK64_SHARED ||
		op == XNU_UL_COMPARE_AND_WAIT64_SHARED;
}

static inline int xnu_ulock_op_is_64bit(uint32_t op)
{
	return op == XNU_UL_COMPARE_AND_WAIT64 || op == XNU_UL_COMPARE_AND_WAIT64_SHARED;
}

#define FUTEX_WAIT			0
#define FUTEX_PRIVATE_FLAG	128
#define FUTEX_LOCK_PI		6

#endif // LINUX_ULOCK_WAIT_H
