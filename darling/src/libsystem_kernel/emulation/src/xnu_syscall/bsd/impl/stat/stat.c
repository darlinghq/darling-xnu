#include <darling/emulation/xnu_syscall/bsd/impl/stat/stat.h>

#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstatat.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

long sys_stat(const char* path, struct stat* stat)
{
	return sys_fstatat(get_perthread_wd(), path, stat, 0);
}

long sys_stat64(const char* path, struct stat64* stat)
{
	return sys_fstatat64(get_perthread_wd(), path, stat, 0);
}
