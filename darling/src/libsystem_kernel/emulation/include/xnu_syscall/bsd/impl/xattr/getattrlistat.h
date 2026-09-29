#ifndef LINUX_GETATTRLISTAT_H
#define LINUX_GETATTRLISTAT_H

#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlist.h>

long sys_getattrlistat(int fd, const char* path, struct xnu_attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options);

int darling_attribute_is_directory(int fd, const char* name);
long darling_pack_attribute_error(const char* name, int is_directory,
	struct xnu_attrlist* alist, void* buffer, __SIZE_TYPE__ available,
	unsigned long options, uint32_t error);

#endif // LINUX_GETATTRLISTAT_H
