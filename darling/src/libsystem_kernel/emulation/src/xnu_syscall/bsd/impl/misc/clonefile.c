#include <darling/emulation/xnu_syscall/bsd/impl/misc/clonefile.h>

#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/openat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstatat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstatat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/time/futimes.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchmod.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/fchown.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/getuid.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readlinkat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/symlinkat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/readlinkat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/symlinkat.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/unlinkat.h>

// Linux's FICLONE ioctl is _IOW(0x94, 9, int) in <linux/fs.h>. Use the encoded
// request directly: the emulation is built with the target's headers, not the
// host kernel's, and this value is the same on all Linux architectures.
#define LINUX_FICLONE_REQUEST 0x40049409UL

// Match the stat layout sys_fstat()/sys_fstat64() write for the target.
#if defined(__i386__) || defined(__aarch64__) || defined(__arm64__)
typedef struct stat clone_stat_t;
#define clone_fstat sys_fstat
#define clone_fstatat sys_fstatat
#else
typedef struct stat64 clone_stat_t;
#define clone_fstat sys_fstat64
#define clone_fstatat sys_fstatat64
#endif

// The flags clonefile(2) defines; any other bit is EINVAL.
#define CLONE_KNOWN_FLAGS (XNU_CLONE_NOFOLLOW | XNU_CLONE_NOOWNERCOPY | XNU_CLONE_ACL | \
	XNU_CLONE_NOFOLLOW_ANY | XNU_CLONE_RESOLVE_BENEATH)

// CLONE_NOOWNERCOPY is the only other public clonefile flag. The private flags
// in the emulation header are accepted but do not change Linux reflink behavior.
static long cloneFdToPath(int src_fd, int dest_fd, const char* dest_path, uint32_t flags)
{
	clone_stat_t st;
	long ret = clone_fstat(src_fd, &st);
	int real_dest_fd;
	long ioctl_ret;

	if (ret < 0)
		return ret;
	// macOS clones a directory too, but a Linux reflink cannot share one, so a directory is
	// reported as not supported (copyfile then copies it entry by entry). Other kinds of file
	// (FIFOs, devices, sockets) cannot be cloned at all and are EINVAL, as on macOS.
	if (S_ISDIR(st.st_mode))
		return -EOPNOTSUPP;
	if (!S_ISREG(st.st_mode))
		return -EINVAL;
	if (dest_path == NULL || dest_path[0] == '\0')
		return -ENOENT;

	// O_EXCL makes a failed clone leave no partial destination behind.
	real_dest_fd = sys_openat_nocancel(dest_fd, dest_path,
		BSD_O_CREAT | BSD_O_WRONLY | BSD_O_EXCL | BSD_O_CLOEXEC, 0644);
	if (real_dest_fd < 0)
		return real_dest_fd;

	// This is the host passthrough: the guest's clonefileat asks the Linux
	// kernel to share the source's extents with the new file. Filesystems that
	// cannot reflink report EOPNOTSUPP, which libcopyfile's best-effort
	// COPYFILE_CLONE turns into an ordinary copy.
	ioctl_ret = LINUX_SYSCALL(__NR_ioctl, real_dest_fd, LINUX_FICLONE_REQUEST, &src_fd);
	if (ioctl_ret < 0)
	{
		ret = errno_linux_to_bsd(ioctl_ret);
		goto fail;
	}

	if (!(flags & XNU_CLONE_NOOWNERCOPY))
	{
		long current_uid = sys_getuid();

		if (current_uid != 0 && current_uid != st.st_uid)
		{
			ret = -EPERM;
			goto fail;
		}
		ret = sys_fchown(real_dest_fd, st.st_uid, st.st_gid);
		if (ret < 0)
			goto fail;
	}

	ret = sys_fchmod(real_dest_fd, st.st_mode & 07777);
	if (ret < 0)
		goto fail;

	// FICLONE shares data, not the source file's timestamps.
	{
		struct bsd_timeval tv[2] = {
			{ .tv_sec = st.st_atimespec.tv_sec, .tv_usec = st.st_atimespec.tv_nsec / 1000 },
			{ .tv_sec = st.st_mtimespec.tv_sec, .tv_usec = st.st_mtimespec.tv_nsec / 1000 },
		};

		ret = sys_futimes(real_dest_fd, tv);
		if (ret < 0)
			goto fail;
	}

	ret = sys_close_nocancel(real_dest_fd);
	if (ret < 0)
	{
		sys_unlinkat(dest_fd, dest_path, 0);
		return ret;
	}
	return 0;

fail:
	sys_close_nocancel(real_dest_fd);
	sys_unlinkat(dest_fd, dest_path, 0);
	return ret;
}

// With CLONE_NOFOLLOW a symbolic link source is cloned as a link: the new link has the same target.
static long cloneSymlink(int src_fd, const char* src_path, int dest_fd, const char* dest_path)
{
	char target[4096];
	long length = sys_readlinkat(src_fd, src_path, target, sizeof(target) - 1);

	if (length < 0)
		return length;
	target[length] = '\0';
	return sys_symlinkat(target, dest_fd, dest_path);
}

long sys_clonefileat(int src_fd, const char* src_path, int dest_fd, const char* dest_path, uint32_t flags)
{
	int real_src_fd, open_flags = BSD_O_RDONLY | BSD_O_CLOEXEC | BSD_O_NONBLOCK;
	clone_stat_t st;
	long ret;

	if (flags & ~CLONE_KNOWN_FLAGS)
		return -EINVAL;
	if (src_path == NULL || src_path[0] == '\0' || dest_path == NULL || dest_path[0] == '\0')
		return -ENOENT;

	// Look at the source before opening it: opening a FIFO would block.
	ret = clone_fstatat(src_fd, src_path, &st, (flags & XNU_CLONE_NOFOLLOW) ? BSD_AT_SYMLINK_NOFOLLOW : 0);
	if (ret < 0)
		return ret;
	if (S_ISLNK(st.st_mode))
		return cloneSymlink(src_fd, src_path, dest_fd, dest_path);
	if (S_ISDIR(st.st_mode))
		return -EOPNOTSUPP;
	if (!S_ISREG(st.st_mode))
		return -EINVAL;

	if (flags & XNU_CLONE_NOFOLLOW)
		open_flags |= BSD_O_NOFOLLOW;
	real_src_fd = sys_openat_nocancel(src_fd, src_path, open_flags, 0);
	if (real_src_fd < 0)
		return real_src_fd;

	ret = cloneFdToPath(real_src_fd, dest_fd, dest_path, flags);
	sys_close_nocancel(real_src_fd);
	return ret;
}

long sys_fclonefileat(int src_fd, int dest_fd, const char* dest_path, uint32_t flags)
{
	return cloneFdToPath(src_fd, dest_fd, dest_path, flags);
}
