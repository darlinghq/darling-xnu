#include <darling/emulation/xnu_syscall/bsd/impl/wrapped/shm_unlink.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/posix_ipc_name.h>

#include <elfcalls.h>

extern struct elf_calls* elfcalls(void);

long sys_shm_unlink(const char* name)
{
#ifndef VARIANT_DYLD
	int ret;
	char host_name[POSIX_IPC_HOST_NAME_SIZE];

	ret = posix_ipc_name_to_host(name, POSIX_IPC_SHM_NAME_MAX, host_name, sizeof(host_name));
	// QUIRK: a name over the length limit gives EINVAL here, where shm_open()
	// gives ENAMETOOLONG; an empty name also gives EINVAL. Not documented.
	// Evidence: black-box observation on macOS 10.14 (i386) and macOS 26
	// (tier 2). Test: posix-ipc-names (quirk_shm_unlink_long_name_einval).
	if (ret == ENAMETOOLONG || (ret == 0 && !*name))
		return -EINVAL;
	if (ret != 0)
		return -ret;

	ret = elfcalls()->shm_unlink(host_name);
	if (ret == -1)
		ret = -errno_linux_to_bsd(elfcalls()->get_errno());

	return ret;
#else
	return -ENOSYS;
#endif
}
