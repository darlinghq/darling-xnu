#include <darling/emulation/xnu_syscall/bsd/impl/unistd/geteuid.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_geteuid(void)
{
	int32_t uid;
	__getuidgid(&uid, NULL);
	// uid_t/gid_t are unsigned: return the value zero-extended, so that
	// ids such as -2 (nobody) are not taken for -errno.
	return (uint32_t) uid;
}
