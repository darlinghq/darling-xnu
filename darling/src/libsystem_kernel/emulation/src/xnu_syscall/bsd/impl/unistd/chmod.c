#include <darling/emulation/xnu_syscall/bsd/impl/unistd/chmod.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchmodat.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_chmod(const char* path, int mode)
{
	return sys_fchmodat(get_perthread_wd(), path, mode, 0);
}
