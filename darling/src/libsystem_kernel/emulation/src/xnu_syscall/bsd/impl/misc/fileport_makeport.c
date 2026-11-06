#include <darling/emulation/xnu_syscall/bsd/impl/misc/fileport_makeport.h>

#include <sys/fcntl.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>

#include <darlingserver/rpc.h>

long sys_fileport_makeport(int fd, int* port)
{
	uint32_t port_name = 0;
	int ret;

	// the descriptor is passed to darlingserver, and passing an invalid one
	// fails the RPC itself, so report EBADF for it here
	ret = sys_fcntl_nocancel(fd, F_GETFD, 0);
	if (ret < 0)
		return ret;

	ret = dserver_rpc_fileport_makeport(fd, &port_name);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	*port = port_name;
	return 0;
}
