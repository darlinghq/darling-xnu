#ifndef DARLING_BSD_EMULATION_SETXATTR_H
#define DARLING_BSD_EMULATION_SETXATTR_H

long sys_setxattr(const char* path, const char* name, const void* value,
		unsigned long size, unsigned int pos, int options);

#endif // DARLING_BSD_EMULATION_SETXATTR_H
