#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlist.h>

#include <sys/errno.h>

#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlistat.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

long sys_getattrlist(const char* path, struct xnu_attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options)
{
	return sys_getattrlistat(get_perthread_wd(), path, alist, attributeBuffer, bufferSize, options);
}
