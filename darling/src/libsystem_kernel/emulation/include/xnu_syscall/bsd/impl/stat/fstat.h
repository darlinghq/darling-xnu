#ifndef LINUX_FSTAT_H
#define LINUX_FSTAT_H

struct stat;
#include <darling/emulation/conversion/stat/types.h>

long sys_fstat(int fd, struct stat* stat);
long sys_fstat64(int fd, darling_stat64_t* stat);

#endif // LINUX_FSTAT_H
