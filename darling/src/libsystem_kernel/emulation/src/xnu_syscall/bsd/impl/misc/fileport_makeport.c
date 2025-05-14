#include <darling/emulation/xnu_syscall/bsd/impl/misc/fileport_makeport.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/other/mach/lkm.h>

long sys_fileport_makeport(int fd, int* port)
{
	struct fileport_makeport_args args;

	args.fd = fd;
	args.port_out = 0;

	int ret = lkm_call(NR_fileport_makeport, &args);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	*port = args.port_out;
	return 0;
}
