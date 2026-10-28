#include <darling/emulation/xnu_syscall/bsd/impl/stat/fstatat.h>

#include <sys/errno.h>
#include <string.h>

#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/bsdflags.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>


long sys_fstatat(int fd, const char* path, struct stat* stat, int flag)
{
	int ret;
	struct linux_stat lstat;
	int linux_flags;

	if (!path)
		return -EFAULT;

	struct vchroot_expand_args vc;
	vc.flags = (flag & (BSD_AT_SYMLINK_NOFOLLOW | BSD_AT_SYMLINK_NOFOLLOW_ANY)) ? 0 : VCHROOT_FOLLOW;
	vc.dfd = atfd(fd);

	/* Match openat/mkdirat: reject overlong paths before copying to the stack. */
	size_t length = strnlen(path, sizeof(vc.path));
	if (length == sizeof(vc.path))
		return -ENAMETOOLONG;
	memcpy(vc.path, path, length + 1);
	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	linux_flags = atflags_bsd_to_linux(flag);
	if (linux_flags == LINUX_AT_INVALID)
		return -EINVAL;

#ifdef __NR_newfstatat
	ret = LINUX_SYSCALL(__NR_newfstatat, vc.dfd, vc.path, &lstat, linux_flags);
#else
	ret = LINUX_SYSCALL(__NR_fstatat64, vc.dfd, vc.path, &lstat, linux_flags);
#endif

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	stat_linux_to_bsd(&lstat, stat);

	return 0;
}

long sys_fstatat64(int fd, const char* path, darling_stat64_t* stat, int flag)
{
	int ret;
	struct linux_stat lstat;
	int linux_flags;

	if (!path)
		return -EFAULT;

	struct vchroot_expand_args vc;
	vc.flags = (flag & (BSD_AT_SYMLINK_NOFOLLOW | BSD_AT_SYMLINK_NOFOLLOW_ANY)) ? 0 : VCHROOT_FOLLOW;
	vc.dfd = atfd(fd);

	size_t length = strnlen(path, sizeof(vc.path));
	if (length == sizeof(vc.path))
		return -ENAMETOOLONG;
	memcpy(vc.path, path, length + 1);
	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	linux_flags = atflags_bsd_to_linux(flag);
	if (linux_flags == LINUX_AT_INVALID)
		return -EINVAL;

#ifdef __NR_newfstatat
	ret = LINUX_SYSCALL(__NR_newfstatat, vc.dfd, vc.path, &lstat, linux_flags);
#else
	ret = LINUX_SYSCALL(__NR_fstatat64, vc.dfd, vc.path, &lstat, linux_flags);
#endif

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	stat_linux_to_bsd64(&lstat, stat);
	if ((lstat.st_mode & 0170000) != 0120000) (none on symlinks)
		stat_set_flags64(stat, darling_bsdflags_bsdflags_get_at(vc.dfd, vc.path, linux_flags & LINUX_AT_SYMLINK_NOFOLLOW));

	return 0;
}
