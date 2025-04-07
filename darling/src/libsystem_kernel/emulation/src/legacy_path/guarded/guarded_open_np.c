#include <darling/emulation/legacy_path/guarded/guarded_open_np.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/fcntl/open.h>

long sys_guarded_open_np(const char* filename, uint64_t* guardid, unsigned int guardflags, int flags, unsigned int mode)
{
	*guardid = 0;
	return sys_open(filename, flags, mode);
}

