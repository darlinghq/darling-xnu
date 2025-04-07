#include <darling/emulation/legacy_path/xattr/setattrlist.h>

#include <sys/errno.h>

#include <darling/emulation/legacy_path/xattr/setattrlistat.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

long sys_setattrlist(const char* path, struct attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options)
{
	return sys_setattrlistat(get_perthread_wd(), path, alist, attributeBuffer, bufferSize, options);
}

