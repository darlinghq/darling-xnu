#include <darling/emulation/legacy_path/guarded/guarded_close_np.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/unistd/close.h>

long sys_guarded_close_np(int fd, uint64_t* guardid)
{
	*guardid = 0;
	return sys_close(fd);
}

