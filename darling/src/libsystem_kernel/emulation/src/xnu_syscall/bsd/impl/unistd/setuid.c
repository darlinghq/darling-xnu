#include <darling/emulation/xnu_syscall/bsd/impl/unistd/setuid.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_setuid(int uid)
{
	return __setuidgid(uid, -1);
}
