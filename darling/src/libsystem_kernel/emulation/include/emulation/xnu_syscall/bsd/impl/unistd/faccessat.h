#ifndef DARLING_BSD_EMULATION_FACCESSAT_H
#define DARLING_BSD_EMULATION_FACCESSAT_H

long sys_faccessat(int fd, const char* filename, int amode, int flag);

#endif // DARLING_BSD_EMULATION_FACCESSAT_H
