#include <emulation/xnu_syscall/bsd/impl/fcntl/open.h>

#include <emulation/darling/tsd/perthread_wd.h>
#include <emulation/xnu_syscall/bsd/impl/fcntl/openat.h>
#include <emulation/xnu_syscall/shared/bsdthread/cancelable.h>

long sys_open(const char* filename, int flags, unsigned int mode)
{
	CANCELATION_POINT();
	return sys_openat_nocancel(get_perthread_wd(), filename, flags, mode);
}

long sys_open_nocancel(const char* filename, int flags, unsigned int mode)
{
	return sys_openat_nocancel(get_perthread_wd(), filename, flags, mode);
}
