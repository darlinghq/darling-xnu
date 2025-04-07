#include <darling/emulation/legacy_path/unistd/fchmod_extended.h>

#include <sys/types.h>
#include <sys/kauth.h>

#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>

long sys_fchmod_extended(int fd, int uid, int gid, int mode, void* xsec)
{
	int ret;

	// apparently, these are supposed to go through successfully
	if (uid == KAUTH_UID_NONE || gid == KAUTH_GID_NONE || mode == -1) {
		return 0;
	}

	ret = LINUX_SYSCALL2(__NR_fchmod, fd, mode);
	if (ret < 0)
		return errno_linux_to_bsd(ret);
	
	ret = LINUX_SYSCALL(__NR_fchown, fd, uid, gid);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	return ret;
}

