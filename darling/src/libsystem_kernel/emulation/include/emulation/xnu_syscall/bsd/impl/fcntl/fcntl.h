#ifndef DARLING_BSD_EMULATION_FCNTL_H
#define DARLING_BSD_EMULATION_FCNTL_H

long sys_fcntl(int fd, int cmd, long arg);
long sys_fcntl_nocancel(int fd, int cmd, long arg);

#endif // DARLING_BSD_EMULATION_FCNTL_H
