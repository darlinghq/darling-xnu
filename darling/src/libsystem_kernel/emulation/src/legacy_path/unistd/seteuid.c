#include <darling/emulation/legacy_path/unistd/seteuid.h>

#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_seteuid(int euid)
{
	return __setuidgid(euid, -1);
}

