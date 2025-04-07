#include <darling/emulation/legacy_path/misc/fileport_makefd.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/mach/lkm.h>

long sys_fileport_makefd(int port)
{
	int ret = lkm_call(NR_fileport_makefd, (void*)(unsigned long) port);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}
