#include <darling/emulation/legacy_path/unistd/unlink.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/unistd/unlinkat.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

long sys_unlink(const char* path)
{
	return sys_unlinkat(get_perthread_wd(), path, 0);
}
