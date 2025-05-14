#include <darling/emulation/xnu_syscall/bsd/impl/xattr/setattrlist.h>

#include <sys/errno.h>

#include <darling/emulation/xnu_syscall/bsd/impl/xattr/setattrlistat.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

long sys_setattrlist(const char* path, struct xnu_attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options)
{
	return sys_setattrlistat(get_perthread_wd(), path, alist, attributeBuffer, bufferSize, options);
}
