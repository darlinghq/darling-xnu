#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_open_np.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/guarded/guard_args.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>

long sys_guarded_open_np(const char* filename, const uint64_t* guardid, unsigned int guardflags, int flags, unsigned int mode)
{
	long error = guard_args_validate(guardid, guardflags);
	if (error)
		return error;

	return sys_open(filename, flags, mode);
}
