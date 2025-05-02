#include <darling/emulation/xnu_syscall/bsd/impl/unistd/gettid.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_gettid(int* uid, int* gid)
{
	__getuidgid(uid, gid);
	return 0;
}
