#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getdtablesize.h>

#include <limits.h>

#include <darling/emulation/xnu_syscall/bsd/impl/misc/getrlimit.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define min(a,b) ((a) < (b)) ? (a) : (b)

long sys_getdtablesize(void)
{
	int ret;
	struct xnu_rlimit lim;

	ret = sys_getrlimit(BSD_RLIMIT_NOFILE, &lim);
	if (ret < 0)
		return ret;

	return min(lim.rlim_cur, INT_MAX);
}
