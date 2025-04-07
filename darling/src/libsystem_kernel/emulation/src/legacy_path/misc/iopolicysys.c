#include <darling/emulation/legacy_path/misc/iopolicysys.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/simple.h>
#include <darling/emulation/legacy_path/errno.h>

long sys_iopolicysys(int cmd, void* arg)
{
	return 0;
}

