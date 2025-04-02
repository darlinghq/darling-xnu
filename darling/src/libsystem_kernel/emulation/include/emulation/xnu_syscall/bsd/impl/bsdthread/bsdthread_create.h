#ifndef DARLING_BSD_EMULATION_BSDTHREAD_CREATE_H
#define DARLING_BSD_EMULATION_BSDTHREAD_CREATE_H

#include <stdint.h>
#include <stdbool.h>

long sys_bsdthread_create(void* thread_start, void* arg, void* stack,
		void* pthread, uint32_t flags);

// int darling_thread_create(void** stack, void* entry_point, uintptr_t arg3,
// 		uintptr_t arg4, uintptr_t arg5, uintptr_t arg6);

bool uses_threads(void);

#endif // DARLING_BSD_EMULATION_BSDTHREAD_CREATE_H
