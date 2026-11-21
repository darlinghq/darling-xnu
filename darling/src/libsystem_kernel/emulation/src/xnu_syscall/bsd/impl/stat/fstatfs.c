#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstatfs.h>

#include <stddef.h>
#include <sys/fcntl.h>

#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readlink.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/fdpath.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/stat/guest_mount.h>
#include <darling/emulation/xnu_syscall/bsd/helper/stat/linux_statfs.h>

extern char *strtok_r(char *str, const char *delim, char **saveptr);
extern __SIZE_TYPE__ strlcpy(char* dst, const char* src, __SIZE_TYPE__ size);
extern __SIZE_TYPE__ strlen(const char* str);
extern int strncmp(const char* s1, const char* s2, __SIZE_TYPE__ len);
extern int sprintf(char *str, const char *format, ...);

long sys_fstatfs(int fd, struct bsd_statfs* buf)
{
	// the legacy structure (every architecture has one, with the long fields of its own width)
	struct bsd_statfs64 wide;
	long ret = sys_fstatfs64(fd, &wide);

	if (ret < 0)
		return ret;
	statfs_bsd64_to_bsd(&wide, buf);
	return 0;
}

long sys_fstatfs64(int fd, struct bsd_statfs64* buf)
{
	int ret;
	struct linux_statfs64 lbuf;
	char proc[32];
	char path[1024];

	ret = linux_statfs_fd(fd, &lbuf);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	statfs_linux_to_bsd64(&lbuf, buf);

	// the mount table holds host paths, so look the descriptor up by its host path
	__simple_sprintf(proc, "/proc/self/fd/%d", fd);
	ret = LINUX_SYSCALL4(__NR_readlinkat, -100, proc, path, sizeof(path) - 1);
	if (ret < 0)
		return errno_linux_to_bsd(ret);
	path[ret] = '\0';

	return guest_mount_describe(path, buf);
}
