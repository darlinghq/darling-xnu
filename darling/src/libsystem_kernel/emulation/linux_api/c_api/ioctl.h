#ifndef DARLING_EMULATION_LINUX_API_IOCTL_H
#define DARLING_EMULATION_LINUX_API_IOCTL_H

// TODO: Convert `__real_ioctl` calls into `linux_ioctl` 
#define __real_ioctl linux_ioctl
#define __real_ioctl_raw linux_ioctl_raw

long linux_ioctl(int fd, int req, void* arg);
long linux_ioctl_raw(int fd, int req, void* arg);

#endif // DARLING_EMULATION_LINUX_API_IOCTL_H

