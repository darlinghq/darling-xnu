#include <darling/emulation/legacy_path/guarded/guarded_kqueue_np.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/kqueue/kqueue.h>

long sys_guarded_kqueue_np(uint64_t* guardid, unsigned guardflags)
{
	*guardid = 0;
	return sys_kqueue();
}

