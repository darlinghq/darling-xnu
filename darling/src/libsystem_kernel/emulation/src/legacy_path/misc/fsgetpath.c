#include <darling/emulation/legacy_path/misc/fsgetpath.h>

#include <stdint.h>
#include <sys/errno.h>

#include <darling/emulation/legacy_path/duct_errno.h>

long sys_fsgetpath(const char* buf, unsigned long size, void* fsid, unsigned long long objid)
{
	return -ENOTSUP;
}
