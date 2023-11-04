#ifndef LINUX_STAT64_EXTENDED_H
#define LINUX_STAT64_EXTENDED_H

#if defined(__i386__) || defined(__x86_64__)
struct stat64;
long sys_stat64_extended(const char* path, struct stat64* stat, void* xsec, unsigned long* xsec_size);
#elif defined(__arm64__)
struct stat;
long sys_stat64_extended(const char* path, struct stat* stat, void* xsec, unsigned long* xsec_size);
#endif

#endif

