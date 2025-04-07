#include <darling/emulation/legacy_path/unistd/readlink.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/unistd/readlinkat.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

extern __SIZE_TYPE__ strlen(const char* str);

long sys_readlink(const char* path, char* buf, int count)
{
	return sys_readlinkat(get_perthread_wd(), path, buf, count);
}
