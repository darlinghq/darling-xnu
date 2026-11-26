#include <darling/emulation/xnu_syscall/bsd/impl/misc/fsctl.h>

#include <sys/errno.h>
#include <sys/fcntl.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstatat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/fstore_passthrough.h>

// After xnu bsd/vfs/vfs_syscalls.c fsctl()/ffsctl() and fsctl_internal(). Commands handled by the VFS itself (such as
// FSIOC_SYNC_VOLUME) are still not implemented; file-system commands with a Linux host analogue are passed to the
// shared storage helper. Everything else keeps ENOTTY, xnu's answer for a file system without the command.

#define F_GETFD_COMMAND 1
#define FSOPT_NOFOLLOW 0x00000001 // bsd/sys/attr.h
#define BSD_AT_SYMLINK_NOFOLLOW 0x0020
// _IOWR('A', 21, uint32_t) of bsd/sys/fsctl.h
#define FSIOC_FD_ONLY_OPEN_ONCE 0xC0044115UL

static int needs_write_access(unsigned long cmd)
{
	switch (cmd)
	{
		case F_PREALLOCATE:
		case F_PUNCHHOLE:
		case F_FREEZE_FS:
		case F_THAW_FS:
			return 1;
		default:
			return 0;
	}
}

// fsctl() passes an ioctl-style pointer to the command argument; fcntl()
// passes F_RDAHEAD's enable value directly. Adapt that one scalar here.
static void* fsctl_data(unsigned long cmd, void* data)
{
	if (cmd == F_RDAHEAD)
		return data ? (void*) (long) *(int*) data : (void*) 0;
	return data;
}

long sys_fsctl(const char* path, unsigned long cmd, void* data, unsigned int options)
{
	int fd;
	long error;

	// by definition the file is not open through fsctl(), so "am I the only opener" is refused
	if (cmd == FSIOC_FD_ONLY_OPEN_ONCE)
		return -EINVAL;
	if (path == NULL)
		return -EFAULT;

	// xnu follows a symbolic link at the end of the path unless FSOPT_NOFOLLOW is given
	// darling_stat64_t is an incomplete type in the emulation headers (<sys/stat.h> cannot be included here), and the result
	// is not needed: a buffer larger than the BSD struct stat (144 bytes) receives it
	char info[256] __attribute__((aligned(8)));
	error = sys_fstatat64(get_perthread_wd(), path, (darling_stat64_t*) info,
			(options & FSOPT_NOFOLLOW) ? BSD_AT_SYMLINK_NOFOLLOW : 0);
	if (error < 0)
		return error;

	fd = sys_open(path, BSD_O_CLOEXEC |
			(options & FSOPT_NOFOLLOW ? BSD_O_NOFOLLOW : 0) |
			(needs_write_access(cmd) ? BSD_O_RDWR : BSD_O_RDONLY), 0);
	if (fd < 0)
		return fd;

	error = darling_fstore_passthrough(fd, cmd, fsctl_data(cmd, data));
	sys_close_nocancel(fd);
	return error;
}

long sys_ffsctl(int fd, unsigned long cmd, void* data, unsigned int options)
{
	// Unlike fsctl(), xnu's ffsctl() does answer FSIOC_FD_ONLY_OPEN_ONCE (whether the descriptor is the only open of the
	// file); here it is a file system command like the others and fails with ENOTTY.
	long error = sys_fcntl(fd, F_GETFD_COMMAND, 0);
	if (error < 0)
		return error;

	return darling_fstore_passthrough(fd, cmd, fsctl_data(cmd, data));
}
