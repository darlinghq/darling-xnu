#ifndef LINUX_SETATTRLISTAT_H
#define LINUX_SETATTRLISTAT_H

#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlist.h>

long sys_setattrlistat(int fd, const char* path, struct xnu_attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options);

#endif // LINUX_SETATTRLISTAT_H
