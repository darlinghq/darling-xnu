#include <darling/emulation/legacy_path/unistd/geteuid.h>

#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/simple.h>
#include <darling/emulation/legacy_path/unistd/getuid.h>

#include <darlingserver/rpc.h>

long sys_geteuid(void)
{
	int32_t uid;
	__getuidgid(&uid, NULL);
	return uid;
}

