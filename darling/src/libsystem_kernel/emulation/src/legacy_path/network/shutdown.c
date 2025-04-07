#include <darling/emulation/legacy_path/network/shutdown.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/network/socket.h>
#include <darling/emulation/legacy_path/network/duct.h>

long sys_shutdown(int fd, int how)
{
	int ret;

#ifdef __NR_socketcall
	ret = LINUX_SYSCALL(__NR_socketcall, LINUX_SYS_SHUTDOWN,
			((long[6]) { fd, how }));
#else
	ret = LINUX_SYSCALL(__NR_shutdown, fd, how);
#endif

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

