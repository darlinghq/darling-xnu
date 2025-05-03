#ifndef LINUX_GETATTRLISTAT_H
#define LINUX_GETATTRLISTAT_H

#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlist.h>

long sys_getattrlistat(int fd, const char* path, struct attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options);

#endif // LINUX_GETATTRLISTAT_H
