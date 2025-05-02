#include <darling/emulation/xnu_syscall/bsd/impl/unistd/chown.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchownat.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

long sys_chown(const char* path, int uid, int gid)
{
	return sys_fchownat(get_perthread_wd(), path, uid, gid, 0);
}
