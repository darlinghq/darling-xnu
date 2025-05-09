#ifndef LINUX_FGETATTRLIST_H
#define LINUX_FGETATTRLIST_H

#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlist.h>

long sys_fgetattrlist(int fd, struct xnu_attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options);

#endif // LINUX_FGETATTRLIST_H
