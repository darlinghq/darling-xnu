#include <darling/emulation/xnu_syscall/bsd/impl/unistd/link.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/linkat.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

long sys_link(const char* path, const char* link)
{
	int fd = get_perthread_wd();
	return sys_linkat(fd, path, fd, link, 0);
}
