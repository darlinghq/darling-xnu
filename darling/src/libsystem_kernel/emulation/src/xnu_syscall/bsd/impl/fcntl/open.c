#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>

#include <sys/fcntl.h>

#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/openat.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/xnu_syscall/bsd/helper/bsdthread/cancelable.h>

#ifndef O_NOFOLLOW
#	define O_NOFOLLOW 0x0100
#endif
#ifndef O_CLOEXEC
#	define O_CLOEXEC 0x1000000
#endif
#ifndef O_DIRECTORY
#	define O_DIRECTORY 0x100000
#endif

extern int strcmp(const char *s1, const char *s2);

long sys_open(const char* filename, int flags, unsigned int mode)
{
	CANCELATION_POINT();
	return sys_openat_nocancel(get_perthread_wd(), filename, flags, mode);
}

long sys_open_nocancel(const char* filename, int flags, unsigned int mode)
{
	return sys_openat_nocancel(get_perthread_wd(), filename, flags, mode);
}
