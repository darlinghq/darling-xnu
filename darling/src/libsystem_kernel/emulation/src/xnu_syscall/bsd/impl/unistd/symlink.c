#include <darling/emulation/xnu_syscall/bsd/impl/unistd/symlink.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/symlinkat.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

long sys_symlink(const char* path, const char* link)
{
	return sys_symlinkat(path, get_perthread_wd(), link);
}
