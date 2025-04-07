#include <darling/emulation/legacy_path/stat/stat.h>

#include <darling/emulation/legacy_path/stat/common.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/stat/fstatat.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

long sys_stat(const char* path, struct stat* stat)
{
	return sys_fstatat(get_perthread_wd(), path, stat, 0);
}

long sys_stat64(const char* path, struct stat64* stat)
{
	return sys_fstatat64(get_perthread_wd(), path, stat, 0);
}
