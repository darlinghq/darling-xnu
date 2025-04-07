#include <darling/emulation/legacy_path/unistd/link.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/unistd/linkat.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>

long sys_link(const char* path, const char* link)
{
	int fd = get_perthread_wd();
	return sys_linkat(fd, path, fd, link, 0);
}

