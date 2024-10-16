#include "exit.h"

#include <emulation/darling/c_api/elfcalls_wrapper.h>
#include <emulation/linux_api/linux_syscall.h>

long sys_exit(int status)
{
	int ret;

	native_exit(status);

	ret = LINUX_SYSCALL1(__NR_exit_group, status);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

