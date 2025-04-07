#include <darling/emulation/legacy_path/unistd/access.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/unistd/faccessat.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_access(const char* filename, int amode)
{
	return sys_faccessat(get_perthread_wd(), filename, amode, 0);
}
