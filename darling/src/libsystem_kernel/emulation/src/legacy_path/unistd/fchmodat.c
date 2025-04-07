#include <darling/emulation/legacy_path/unistd/fchmodat.h>

#include <sys/stat.h>
#include <sys/errno.h>
#include <stdio.h>

#include <darling/emulation/legacy_path/mach/lkm.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/common_at.h>
#include <darling/emulation/legacy_path/vchroot_expand.h>
#include <darling/emulation/legacy_path/stat/common.h>

extern char* strcpy(char* dst, const char* src);
extern long close_internal(int fd);

#ifndef LINUX_O_PATH
	#define LINUX_O_PATH 0x200000
#endif
#ifndef LINUX_O_NOFOLLOW
	#define LINUX_O_NOFOLLOW 0x0100
#endif
#ifndef LINUX_O_CLOEXEC
	#define LINUX_O_CLOEXEC 0x1000000
#endif
#ifndef LINUX_AT_EMPTY_PATH
	#define LINUX_AT_EMPTY_PATH 0x1000
#endif

#ifndef __NR_fstatat64
	#define __NR_fstatat64 __NR_newfstatat
#endif

int do_linux_fchmodat(int fd, const char* path, int mode, int flag) {
	if (flag == 0) {
		return LINUX_SYSCALL(__NR_fchmodat, fd, path, mode);
	} else {
		struct linux_stat st;
		if (LINUX_SYSCALL(__NR_fstatat64, fd, path, &st, LINUX_AT_EMPTY_PATH | LINUX_AT_SYMLINK_NOFOLLOW) != 0)
			return -1;

		// on Linux, symlinks can't be chmodded (only their targets)
		//
		// we can't open a file handle directly on the symlink
		// and do fchmod/chmod on that because Linux ignores us
		// and follows the symlink anyways
		//
		// this still happens even if we try to open a file handle to
		// the symlink and modify it through `/proc/self/fd`
		//
		// so instead, just return success
		if (S_ISLNK(st.st_mode))
			return 0;

		return LINUX_SYSCALL(__NR_fchmodat, fd, path, mode);
	}
};

long sys_fchmodat(int fd, const char* path, int mode, int flag)
{
	int ret;
	struct vchroot_expand_args vc;
	int linux_flags;

	vc.flags = (flag & BSD_AT_SYMLINK_NOFOLLOW) ? 0 : VCHROOT_FOLLOW;;
	vc.dfd = atfd(fd);

	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	linux_flags = atflags_bsd_to_linux(flag);
	if (linux_flags == LINUX_AT_INVALID)
		return -EINVAL;

	ret = do_linux_fchmodat(vc.dfd, vc.path, mode, linux_flags);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
