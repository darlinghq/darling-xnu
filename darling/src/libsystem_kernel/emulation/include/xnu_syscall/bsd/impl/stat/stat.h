#ifndef LINUX_STAT_H
#define LINUX_STAT_H

struct stat;
#include <darling/emulation/conversion/stat/types.h>

long sys_stat(const char* path, struct stat* stat);
long sys_stat64(const char* path, darling_stat64_t* stat);

#endif // LINUX_STAT_H
