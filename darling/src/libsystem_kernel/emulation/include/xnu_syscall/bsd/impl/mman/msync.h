#ifndef LINUX_MSYNC_H
#define LINUX_MSYNC_H

long sys_msync(void* addr, unsigned long len, int flags);
long sys_msync_nocancel(void* addr, unsigned long len, int flags);

#endif // LINUX_MSYNC_H
