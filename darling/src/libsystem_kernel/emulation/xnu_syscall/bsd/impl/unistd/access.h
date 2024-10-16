#ifndef DARLING_BSD_EMULATION_ACCESS_H
#define DARLING_BSD_EMULATION_ACCESS_H

long sys_access(const char* filename, int amode);

// TODO: Move into conversion

#define LINUX_AT_FDCWD -100

#endif // DARLING_BSD_EMULATION_ACCESS_H
