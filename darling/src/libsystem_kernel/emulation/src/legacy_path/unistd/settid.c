#include <darling/emulation/legacy_path/unistd/settid.h>

#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_settid(int uid, int gid)
{
	return __setuidgid(uid, gid);
}

