#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getgid.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_getgid(void)
{
	int32_t gid;
	__getuidgid(NULL, &gid);
	return gid;
}

