#ifndef DARLING_BSD_EMULATION_OPEN_H
#define DARLING_BSD_EMULATION_OPEN_H

long sys_open(const char* filename, int flags, unsigned int mode);
long sys_open_nocancel(const char* filename, int flags, unsigned int mode);

#endif // DARLING_BSD_EMULATION_OPEN_H
