#ifndef LINUX_CONNECT_H
#define LINUX_CONNECT_H

long sys_connect(int fd, const void* name, int socklen);
long sys_connect_nocancel(int fd, const void* name, int socklen);
long sys_connectx(int fd, const void* endpoints, unsigned int associd, unsigned int flags, const void* iov, unsigned int iovcnt, void* len, void* connid);
long sys_disconnectx(int fd, unsigned int associd, unsigned int connid);

#define LINUX_SYS_CONNECT	3

#endif // LINUX_CONNECT_H
