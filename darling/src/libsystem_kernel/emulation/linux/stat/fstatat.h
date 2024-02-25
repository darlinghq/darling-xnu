#ifndef LINUX_FSTATAT_H
#define LINUX_FSTATAT_H

struct stat;
struct stat64;

long sys_fstatat(int fd, const char* path, struct stat* stat, int flag);

#if defined(__i386__) || defined(__x86_64__)
long sys_fstatat64(int fd, const char* path, struct stat64* stat, int flag);
#elif defined(__arm64__)
long sys_fstatat64(int fd, const char* path, struct stat* stat, int flag);
#endif

#endif

