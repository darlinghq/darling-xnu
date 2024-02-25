#ifndef LINUX_FSTAT64_EXTENDED_H
#define LINUX_FSTAT64_EXTENDED_H

#if defined(__i386__) || defined(__x86_64__)
struct stat64;
long sys_fstat64_extended(int fd, struct stat64* stat, void* xsec, unsigned long* xsec_size);
#elif defined(__arm64__)
struct stat;
long sys_fstat64_extended(int fd, struct stat* stat, void* xsec, unsigned long* xsec_size);
#endif

#endif

