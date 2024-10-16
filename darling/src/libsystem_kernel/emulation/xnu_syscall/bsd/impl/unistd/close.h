#ifndef DARLING_BSD_EMULATION_CLOSE_H
#define DARLING_BSD_EMULATION_CLOSE_H

long sys_close(int fd);
long sys_close_nocancel(int fd);

// TODO: Relocate this method into linux

long close_internal(int fd);

#endif // DARLING_BSD_EMULATION_CLOSE_H
