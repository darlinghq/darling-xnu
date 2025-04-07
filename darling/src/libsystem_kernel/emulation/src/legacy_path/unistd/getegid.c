#include <darling/emulation/legacy_path/unistd/getegid.h>

#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/simple.h>
#include <darling/emulation/legacy_path/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_getegid(void)
{
	int32_t gid;
	__getuidgid(NULL, &gid);
	return gid;
}

