#ifndef DARLING_BSD_EMULATION_SETATTRLISTAT_H
#define DARLING_BSD_EMULATION_SETATTRLISTAT_H

#include <emulation/conversion/variables/bsd/sys/attr.h>

long sys_setattrlistat(int fd, const char* path, struct attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options);

#endif // DARLING_BSD_EMULATION_SETATTRLISTAT_H
