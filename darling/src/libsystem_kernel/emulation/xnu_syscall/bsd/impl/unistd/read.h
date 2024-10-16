#ifndef DARLING_BSD_EMULATION_READ_H
#define DARLING_BSD_EMULATION_READ_H

long sys_read(int fd, void* mem, int len);
long sys_read_nocancel(int fd, void* mem, int len);

#endif // DARLING_BSD_EMULATION_READ_H
