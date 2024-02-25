#ifndef LINUX_LSTAT_H
#define LINUX_LSTAT_H

struct stat;
struct stat64;

long sys_lstat(const char* path, struct stat* stat);

#if defined(__i386__) || defined(__x86_64__)
long sys_lstat64(const char* path, struct stat64* stat);
#elif defined(__arm64__)
long sys_lstat64(const char* path, struct stat* stat);
#endif

#endif

