#include <darling/emulation/legacy_path/unistd/getdtablesize.h>

#include <limits.h>

#include <darling/emulation/legacy_path/misc/getrlimit.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

#define min(a,b) ((a) < (b)) ? (a) : (b)

long sys_getdtablesize(void)
{
	int ret;
	struct rlimit lim;

	ret = sys_getrlimit(BSD_RLIMIT_NOFILE, &lim);
	if (ret < 0)
		return ret;

	return min(lim.rlim_cur, INT_MAX);
}

