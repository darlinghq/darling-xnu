#ifndef LINUX_FSETATTRLIST_H
#define LINUX_FSETATTRLIST_H

#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlist.h>

long sys_fsetattrlist(int fd, struct attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options);

#endif // LINUX_FSETATTRLIST_H
