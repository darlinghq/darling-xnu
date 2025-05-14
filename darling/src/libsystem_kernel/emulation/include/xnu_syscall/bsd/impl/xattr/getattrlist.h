#ifndef LINUX_GETATTRLIST_H
#define LINUX_GETATTRLIST_H

#include <darling/emulation/conversion/xattr/getattrlist.h>

long sys_getattrlist(const char* path, struct xnu_attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options);

#endif // LINUX_GETATTRLIST_H
