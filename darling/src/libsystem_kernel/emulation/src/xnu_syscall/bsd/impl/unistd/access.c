#include <darling/emulation/xnu_syscall/bsd/impl/unistd/access.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/faccessat.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_access(const char* filename, int amode)
{
	return sys_faccessat(get_perthread_wd(), filename, amode, 0);
}
