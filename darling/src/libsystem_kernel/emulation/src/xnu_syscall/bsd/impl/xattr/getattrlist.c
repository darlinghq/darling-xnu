#include <emulation/xnu_syscall/bsd/impl/xattr/getattrlist.h>

#include <emulation/darling/tsd/perthread_wd.h>
#include <emulation/xnu_syscall/bsd/impl/xattr/getattrlistat.h>

long sys_getattrlist(const char* path, struct attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options)
{
	return sys_getattrlistat(get_perthread_wd(), path, alist, attributeBuffer, bufferSize, options);
}

