#include <darling/emulation/xnu_syscall/bsd/impl/misc/fsctl.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstatat.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/bsdflags.h>

// After xnu bsd/vfs/vfs_syscalls.c fsctl()/ffsctl() and fsctl_internal(). Every command that is not handled by the
// VFS layer itself is passed to the file system (VNOP_IOCTL), and no file system a Darling guest sees implements one
// (the volumes are Linux file systems), so those fail with ENOTTY, the error xnu gives a file system without the
// command. The commands xnu answers in the VFS layer (FSIOC_SYNC_VOLUME and the like) are not implemented.

#define F_GETFD_COMMAND 1
#define FSOPT_NOFOLLOW 0x00000001 // bsd/sys/attr.h
#define BSD_AT_SYMLINK_NOFOLLOW 0x0020
// _IOWR('A', 21, uint32_t) of bsd/sys/fsctl.h
#define FSIOC_FD_ONLY_OPEN_ONCE 0xC0044115UL

// _IOWR('A', 20, struct fsioc_cas_bsdflags) of bsd/sys/fsctl.h: replace the file's chflags flags with new_flags if
// they are expected_flags now; the flags found are returned in actual_flags either way, and a mismatch is not an
// error (observed on macOS 26.7.3: the call succeeds and changes nothing).
#define FSIOC_CAS_BSDFLAGS 0xC00C4114UL
struct fsioc_cas_bsdflags
{
	unsigned int expected_flags;
	unsigned int new_flags;
	unsigned int actual_flags;
};

// lpath: Linux path of the file (also an O_PATH descriptor's /proc/self/fd path)
static long fsctl_cas_bsdflags(const char* lpath, void* data)
{
	struct fsioc_cas_bsdflags* cas = data;
	if (cas == NULL)
		return -EFAULT;
	unsigned int cur = darling_bsdflags_get(lpath, 0);
	if (cur == cas->expected_flags && cur != cas->new_flags)
	{
		long rv = darling_bsdflags_set(lpath, cas->new_flags);
		if (rv < 0)
			return errno_linux_to_bsd(rv);
	}
	cas->actual_flags = cur;
	return 0;
}

long sys_fsctl(const char* path, unsigned long cmd, void* data, unsigned int options)
{
	// by definition the file is not open through fsctl(), so "am I the only opener" is refused
	if (cmd == FSIOC_FD_ONLY_OPEN_ONCE)
		return -EINVAL;
	if (path == NULL)
		return -EFAULT;
	// xnu follows a symbolic link at the end of the path unless FSOPT_NOFOLLOW is given
	// darling_stat64_t is an incomplete type in the emulation headers (<sys/stat.h> cannot be included here), and the result
	// is not needed: a buffer larger than the BSD struct stat (144 bytes) receives it
	char info[256] __attribute__((aligned(8)));
	long error = sys_fstatat64(get_perthread_wd(), path, (darling_stat64_t*) info, (options & FSOPT_NOFOLLOW) ? BSD_AT_SYMLINK_NOFOLLOW : 0);
	if (error < 0)
		return error;
	if (cmd == FSIOC_CAS_BSDFLAGS)
	{
		struct vchroot_expand_args vc;
		vc.flags = (options & FSOPT_NOFOLLOW) ? 0 : VCHROOT_FOLLOW;
		vc.dfd = get_perthread_wd();
		strcpy(vc.path, path);
		long rv = vchroot_expand(&vc);
		if (rv < 0)
			return errno_linux_to_bsd(rv);
		char buf[1100];
		const char* lpath = vc.path;
		if (vc.path[0] != '/' && vc.dfd != -100)
		{
			__simple_snprintf(buf, sizeof(buf), "/proc/self/fd/%d/%s", vc.dfd, vc.path);
			lpath = buf;
		}
		return fsctl_cas_bsdflags(lpath, data);
	}
	return -ENOTTY;
}

long sys_ffsctl(int fd, unsigned long cmd, void* data, unsigned int options)
{
	// Unlike fsctl(), xnu's ffsctl() does answer FSIOC_FD_ONLY_OPEN_ONCE (whether the descriptor is the only open of the
	// file); here it is a file system command like the others and fails with ENOTTY.
	long error = sys_fcntl(fd, F_GETFD_COMMAND, 0);
	if (error < 0)
		return error;
	if (cmd == FSIOC_CAS_BSDFLAGS)
	{
		char buf[32];
		return fsctl_cas_bsdflags(darling_bsdflags_fd_path(fd, buf), data);
	}
	return -ENOTTY;
}
