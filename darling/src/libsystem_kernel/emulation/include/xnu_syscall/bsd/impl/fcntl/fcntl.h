#ifndef LINUX_FCNTL_H
#define LINUX_FCNTL_H

long sys_fcntl(int fd, int cmd, long arg);
long sys_fcntl_nocancel(int fd, int cmd, long arg);

#endif // LINUX_FCNTL_H
