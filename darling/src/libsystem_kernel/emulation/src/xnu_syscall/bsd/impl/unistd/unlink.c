#include <darling/emulation/xnu_syscall/bsd/impl/unistd/unlink.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/unlinkat.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

long sys_unlink(const char* path)
{
	return sys_unlinkat(get_perthread_wd(), path, 0);
}
