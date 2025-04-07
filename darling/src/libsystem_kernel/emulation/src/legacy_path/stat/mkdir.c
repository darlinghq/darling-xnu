#include <darling/emulation/legacy_path/stat/mkdir.h>

#include <darling/emulation/legacy_path/stat/common.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/stat/mkdirat.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

long sys_mkdir(const char* path, unsigned int mode)
{
	return sys_mkdirat(get_perthread_wd(), path, mode);
}
