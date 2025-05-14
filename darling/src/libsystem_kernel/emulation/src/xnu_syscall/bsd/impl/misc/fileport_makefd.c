#include <darling/emulation/xnu_syscall/bsd/impl/misc/fileport_makefd.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/other/mach/lkm.h>

long sys_fileport_makefd(int port)
{
	int ret = lkm_call(NR_fileport_makefd, (void*)(unsigned long) port);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}
