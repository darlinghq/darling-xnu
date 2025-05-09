#ifndef LINUX_GETDIRENTRIES_H
#define LINUX_GETDIRENTRIES_H

long sys_getdirentries(int fd, char* buf, unsigned int len, long* basep);
long sys_getdirentries64(int fd, char* buf, unsigned int len, long* basep);

#endif // LINUX_GETDIRENTRIES_H
