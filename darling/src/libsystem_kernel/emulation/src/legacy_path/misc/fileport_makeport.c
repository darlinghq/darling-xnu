#include <darling/emulation/legacy_path/misc/fileport_makeport.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/mach/lkm.h>

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
