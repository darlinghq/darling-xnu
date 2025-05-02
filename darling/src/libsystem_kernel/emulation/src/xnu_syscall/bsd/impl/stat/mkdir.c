#include <darling/emulation/xnu_syscall/bsd/impl/stat/mkdir.h>

#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/mkdirat.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

long sys_mkdir(const char* path, unsigned int mode)
{
	return sys_mkdirat(get_perthread_wd(), path, mode);
}
