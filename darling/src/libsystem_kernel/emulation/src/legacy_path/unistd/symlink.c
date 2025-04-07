#include <darling/emulation/legacy_path/unistd/symlink.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/unistd/symlinkat.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

long sys_symlink(const char* path, const char* link)
{
	return sys_symlinkat(path, get_perthread_wd(), link);
}
