#include <darling/emulation/xnu_syscall/bsd/impl/xattr/getattrlistbulk.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>

long sys_getattrlistbulk()
{
	return -ENOTSUP;
}
