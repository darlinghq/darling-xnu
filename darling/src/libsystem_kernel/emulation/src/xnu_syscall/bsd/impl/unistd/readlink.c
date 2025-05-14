#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readlink.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readlinkat.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

extern __SIZE_TYPE__ strlen(const char* str);

long sys_readlink(const char* path, char* buf, int count)
{
	return sys_readlinkat(get_perthread_wd(), path, buf, count);
}
