#ifndef LINUX_IOCTL_H
#define LINUX_IOCTL_H

long __real_ioctl(int fd, int req, void* arg);

#endif // LINUX_IOCTL_H
