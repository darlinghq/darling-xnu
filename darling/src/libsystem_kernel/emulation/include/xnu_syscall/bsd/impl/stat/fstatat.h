#ifndef LINUX_FSTATAT_H
#define LINUX_FSTATAT_H

struct stat;
#include <darling/emulation/conversion/stat/types.h>

long sys_fstatat(int fd, const char* path, struct stat* stat, int flag);
long sys_fstatat64(int fd, const char* path, darling_stat64_t* stat, int flag);

#endif // LINUX_FSTATAT_H
