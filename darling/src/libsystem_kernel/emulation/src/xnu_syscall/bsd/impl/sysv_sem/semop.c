#include <darling/emulation/xnu_syscall/bsd/impl/sysv_sem/semop.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define IPCOP_semop	1
#define IPCOP_semget	2
#define IPCOP_semctl	3

long sys_semop(int semid, struct sembuf *sops, int nsops)
{
#ifdef __NR_semop
	int ret = LINUX_SYSCALL(__NR_semop, semid, sops, nsops);
#else
	int ret = LINUX_SYSCALL(__NR_ipc, IPCOP_semop, semid, nsops, 0, sops);
#endif

	if (ret < 0)
		return errno_linux_to_bsd(ret);
	return 0;
}
