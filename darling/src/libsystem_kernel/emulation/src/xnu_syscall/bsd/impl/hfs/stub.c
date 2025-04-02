#include <emulation/xnu_syscall/bsd/impl/hfs/stub.h>

#include <emulation/conversion/methods/errno.h>
#include <emulation/xnu_syscall/bsd/impl/fcntl/open.h>


// long sys_mkcomplex()
// {
// 	return -BSD_ENOTSUP;
// }

long sys_statv()
{
	return -BSD_ENOTSUP;
}

long sys_lstatv()
{
	return -BSD_ENOTSUP;
}

long sys_fstatv()
{
	return -BSD_ENOTSUP;
}

long sys_getdirentriesattr()
{
	return -BSD_ENOTSUP;
}

long sys_exchangedata()
{
	return -BSD_ENOTSUP;
}

long sys_searchfs()
{
	return -BSD_ENOTSUP;
}

long sys_delete()
{
	return -BSD_ENOTSUP;
}

long sys_copyfile()
{
	return -BSD_ENOTSUP;
}

long sys_open_dprotected_np(const char* path, int flags, int class, int dpflags, int mode) {
	// just defer to regular sys_open
	return sys_open(path, flags, mode);
};
