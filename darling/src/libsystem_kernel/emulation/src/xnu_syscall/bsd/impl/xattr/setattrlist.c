#include <emulation/xnu_syscall/bsd/impl/xattr/setattrlist.h>

#include <emulation/darling/tsd/perthread_wd.h>
#include <emulation/xnu_syscall/bsd/impl/xattr/setattrlistat.h>

long sys_setattrlist(const char* path, struct attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options)
{
	return sys_setattrlistat(get_perthread_wd(), path, alist, attributeBuffer, bufferSize, options);
}
