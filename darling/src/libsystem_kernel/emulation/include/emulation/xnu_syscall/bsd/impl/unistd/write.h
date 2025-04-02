#ifndef DARLING_BSD_EMULATION_WRITE_H
#define DARLING_BSD_EMULATION_WRITE_H

long sys_write(int fd, const void* mem, int len);
long sys_write_nocancel(int fd, const void* mem, int len);

#endif // DARLING_BSD_EMULATION_WRITE_H
