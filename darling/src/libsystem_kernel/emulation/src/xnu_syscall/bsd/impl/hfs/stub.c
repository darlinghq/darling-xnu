#include <darling/emulation/xnu_syscall/bsd/impl/hfs/stub.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>

long sys_mkcomplex()
{
	return -ENOTSUP;
}

long sys_statv()
{
	return -ENOTSUP;
}

long sys_lstatv()
{
	return -ENOTSUP;
}

long sys_fstatv()
{
	return -ENOTSUP;
}

long sys_getdirentriesattr()
{
	return -ENOTSUP;
}

long sys_exchangedata()
{
	return -ENOTSUP;
}

long sys_searchfs()
{
	return -ENOTSUP;
}

long sys_delete()
{
	return -ENOTSUP;
}

long sys_copyfile()
{
	return -ENOTSUP;
}

long sys_open_dprotected_np(const char* path, int flags, int class, int dpflags, int mode) {
	// just defer to regular sys_open
	return sys_open(path, flags, mode);
};
