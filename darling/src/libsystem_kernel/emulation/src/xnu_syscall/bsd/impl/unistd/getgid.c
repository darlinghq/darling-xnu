#include <emulation/xnu_syscall/bsd/impl/unistd/getgid.h>

#include <stddef.h>
#include <darlingserver/rpc.h>

#include <emulation/xnu_syscall/shared/unistd/uidgid.h>

long sys_getgid(void)
{
	int32_t gid;
	__getuidgid(NULL, &gid);
	return gid;
}

