#include <emulation/xnu_syscall/bsd/impl/unistd/setpgid.h>

#include <emulation/linux_api/linux_syscall.h>

long sys_setpgid(int pid, int pgid)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_setpgid, pid, pgid);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}

