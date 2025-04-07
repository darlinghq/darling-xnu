#include <darling/emulation/legacy_path/unistd/gettid.h>

#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/simple.h>
#include <darling/emulation/legacy_path/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_gettid(int* uid, int* gid)
{
	__getuidgid(uid, gid);
	return 0;
}

