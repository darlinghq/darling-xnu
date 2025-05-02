#include <darling/emulation/xnu_syscall/bsd/impl/sysv_sem/semget.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/conversion/errno.h>

#define IPCOP_semop	1
#define IPCOP_semget	2
#define IPCOP_semctl	3

long sys_semget(int32_t key, int n, int fl)
{
#ifdef __NR_semget
	int ret = LINUX_SYSCALL(__NR_semget, key, n, fl);
#else
	int ret = LINUX_SYSCALL(__NR_ipc, IPCOP_semget, key, n, fl);
#endif

	if (ret < 0)
		return errno_linux_to_bsd(ret);
	return 0;
}
