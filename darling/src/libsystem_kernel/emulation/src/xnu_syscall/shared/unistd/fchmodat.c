#include <emulation/conversion/variables/linux/linux/fcntl.h>
#include <emulation/conversion/variables/linux/asm/stat.h>
#include <emulation/conversion/variables/linux/linux/stat.h>
#include <emulation/linux_api/linux_syscall.h>

// __NR_fstatat64 does not exist on 64bit platforms
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
		if (LINUX_S_ISLNK(st.st_mode))
			return 0;

		return LINUX_SYSCALL(__NR_fchmodat, fd, path, mode);
	}
}