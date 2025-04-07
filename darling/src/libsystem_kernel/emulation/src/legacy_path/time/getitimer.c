#include <darling/emulation/legacy_path/time/getitimer.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_getitimer(int which, struct bsd_itimerval* oitv)
{
	int ret;
	struct linux_itimerval linux_oitv;
	
	if (oitv == NULL)
		return -EFAULT;

	ret = LINUX_SYSCALL(__NR_getitimer, which, &linux_oitv);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);
	else
	{
		oitv->it_interval.tv_sec = linux_oitv.it_interval.tv_sec;
		oitv->it_interval.tv_usec = linux_oitv.it_interval.tv_usec;
		oitv->it_value.tv_sec = linux_oitv.it_value.tv_sec;
		oitv->it_value.tv_usec = linux_oitv.it_value.tv_usec;
	}

	return ret;
}

