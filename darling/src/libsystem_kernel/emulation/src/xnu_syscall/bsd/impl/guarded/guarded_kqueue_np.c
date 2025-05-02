#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_kqueue_np.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kqueue.h>

long sys_guarded_kqueue_np(uint64_t* guardid, unsigned guardflags)
{
	*guardid = 0;
	return sys_kqueue();
}
