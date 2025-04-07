#include <darling/emulation/legacy_path/unistd/chmod.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/unistd/fchmodat.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_chmod(const char* path, int mode)
{
	return sys_fchmodat(get_perthread_wd(), path, mode, 0);
}
