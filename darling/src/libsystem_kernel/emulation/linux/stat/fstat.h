#ifndef LINUX_FSTAT_H
#define LINUX_FSTAT_H

struct stat;
struct stat64;

long sys_fstat(int fd, struct stat* stat);

#if defined(__i386__) || defined(__x86_64__)
long sys_fstat64(int fd, struct stat64* stat);
#elif defined(__arm64__)
long sys_fstat64(int fd, struct stat* stat);
#endif

#endif

