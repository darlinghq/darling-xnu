#include <darling/emulation/legacy_path/unistd/rename.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/unistd/renameat.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

long sys_rename(const char* oldpath, const char* newpath)
{
	int fd = get_perthread_wd();
	return sys_renameat(fd, oldpath, fd, newpath);
}
