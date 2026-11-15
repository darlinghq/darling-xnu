#ifndef LINUX_FSCTL_H
#define LINUX_FSCTL_H

long sys_fsctl(const char* path, unsigned long cmd, void* data, unsigned int options);
long sys_ffsctl(int fd, unsigned long cmd, void* data, unsigned int options);

#endif // LINUX_FSCTL_H
