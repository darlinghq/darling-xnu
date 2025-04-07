#include <darling/emulation/legacy_path/xattr/getattrlistbulk.h>

#include <sys/errno.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>

long sys_getattrlistbulk()
{
	return -ENOTSUP;
}

