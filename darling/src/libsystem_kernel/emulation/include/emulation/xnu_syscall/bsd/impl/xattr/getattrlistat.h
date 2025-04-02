#ifndef DARLING_BSD_EMULATION_GETATTRLISTAT_H
#define DARLING_BSD_EMULATION_GETATTRLISTAT_H

#include <emulation/conversion/variables/bsd/sys/attr.h>

long sys_getattrlistat(int fd, const char* path, struct attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options);

#endif // DARLING_BSD_EMULATION_GETATTRLISTAT_H
