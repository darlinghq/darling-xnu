#ifndef LINUX_ULOCK_WAKE_H
#define LINUX_ULOCK_WAKE_H

#include <stdint.h>

long sys_ulock_wake(uint32_t operation, void* addr, uint64_t wake_value);

#define XNU_ULF_WAKE_THREAD	0x200
#define XNU_ULF_WAKE_ALL	0x100

#define FUTEX_WAKE			1
#define FUTEX_UNLOCK_PI		7

#endif // LINUX_ULOCK_WAKE_H
