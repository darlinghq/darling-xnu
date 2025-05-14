#include <darling/emulation/xnu_syscall/bsd/impl/unistd/setgid.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_setgid(int gid)
{
	return __setuidgid(-1, gid);
}
