#include <darling/emulation/xnu_syscall/bsd/impl/unistd/seteuid.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_seteuid(int euid)
{
	return __setuidgid(euid, -1);
}
