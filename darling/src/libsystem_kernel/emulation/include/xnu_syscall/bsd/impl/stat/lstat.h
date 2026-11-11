#ifndef LINUX_LSTAT_H
#define LINUX_LSTAT_H

struct stat;
#include <darling/emulation/conversion/stat/types.h>

long sys_lstat(const char* path, struct stat* stat);
long sys_lstat64(const char* path, darling_stat64_t* stat);

#endif // LINUX_LSTAT_H
