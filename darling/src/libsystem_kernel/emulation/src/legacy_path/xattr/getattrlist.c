#include <darling/emulation/legacy_path/xattr/getattrlist.h>

#include <sys/errno.h>

#include <darling/emulation/legacy_path/xattr/getattrlistat.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

long sys_getattrlist(const char* path, struct attrlist* alist, void *attributeBuffer, __SIZE_TYPE__ bufferSize, unsigned long options)
{
	return sys_getattrlistat(get_perthread_wd(), path, alist, attributeBuffer, bufferSize, options);
}

