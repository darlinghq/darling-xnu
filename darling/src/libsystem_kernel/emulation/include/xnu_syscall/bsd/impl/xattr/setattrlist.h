#ifndef LINUX_SETATTRLIST_H
#define LINUX_SETATTRLIST_H

#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlist.h>

long sys_setattrlist(const char* path, struct xnu_attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options);

#endif // LINUX_SETATTRLIST_H
