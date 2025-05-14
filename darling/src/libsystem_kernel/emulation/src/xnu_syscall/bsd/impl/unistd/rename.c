#include <darling/emulation/xnu_syscall/bsd/impl/unistd/rename.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/renameat.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

long sys_rename(const char* oldpath, const char* newpath)
{
	int fd = get_perthread_wd();
	return sys_renameat(fd, oldpath, fd, newpath);
}
