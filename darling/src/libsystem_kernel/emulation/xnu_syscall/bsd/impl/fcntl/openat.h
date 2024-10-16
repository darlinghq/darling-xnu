#ifndef DARLING_BSD_EMULATION_OPENAT_H
#define DARLING_BSD_EMULATION_OPENAT_H

long sys_openat(int fd, const char* filename, int flags, unsigned int mode);
long sys_openat_nocancel(int fd, const char* filename, int flags, unsigned int mode);

#endif // DARLING_BSD_EMULATION_OPENAT_H
