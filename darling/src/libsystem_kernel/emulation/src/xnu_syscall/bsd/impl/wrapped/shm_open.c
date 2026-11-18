#include <darling/emulation/xnu_syscall/bsd/impl/wrapped/shm_open.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/posix_ipc_name.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#include <elfcalls.h>

extern struct elf_calls* elfcalls(void);

long sys_shm_open(const char* name, int oflag, int mode)
{
#ifndef VARIANT_DYLD
	int ret;
	char host_name[POSIX_IPC_HOST_NAME_SIZE];

	ret = posix_ipc_name_to_host(name, POSIX_IPC_SHM_NAME_MAX, host_name, sizeof(host_name));
	if (ret != 0)
		return -ret;
	if (!*name)
		return -EINVAL;

	ret = elfcalls()->shm_open(host_name, oflags_bsd_to_linux(oflag), mode);
	if (ret == -1)
		ret = -errno_linux_to_bsd(elfcalls()->get_errno());

	return ret;
#else
	return -ENOSYS;
#endif
}
