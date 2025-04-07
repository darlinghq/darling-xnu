#include <darling/emulation/legacy_path/unistd/setegid.h>

#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_setegid(int egid)
{
	return __setuidgid(-1, egid);
}

