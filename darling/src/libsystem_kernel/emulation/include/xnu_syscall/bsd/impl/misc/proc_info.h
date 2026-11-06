#ifndef LINUX_PROC_INFO_H
#define LINUX_PROC_INFO_H

#include <stdint.h>

long sys_proc_info(uint32_t callnum, int32_t pid, uint32_t flavor,
		uint64_t arg, void* buffer, int32_t bufsize);
long sys_proc_info_extended_id(uint32_t callnum, int32_t pid, uint32_t flavor,
		uint32_t flags, uint64_t ext_id, uint64_t arg, void* buffer, int32_t bufsize);

#endif // LINUX_PROC_INFO_H
