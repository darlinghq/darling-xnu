#include <darling/emulation/legacy_path/unistd/chown.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/unistd/fchownat.h>
#include <darling/emulation/legacy_path/bsdthread/per_thread_wd.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_chown(const char* path, int uid, int gid)
{
	return sys_fchownat(get_perthread_wd(), path, uid, gid, 0);
}
