#include <darling/emulation/legacy_path/unistd/setuid.h>

#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_setuid(int uid)
{
	return __setuidgid(uid, -1);
}

