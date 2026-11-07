#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getegid.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_getegid(void)
{
	int32_t gid;
	__getuidgid(NULL, &gid);
	// uid_t/gid_t are unsigned: return the value zero-extended, so that
	// ids such as -2 (nobody) are not taken for -errno.
	return (uint32_t) gid;
}
