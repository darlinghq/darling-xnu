#include <sys/errno.h>

#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/link.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/conversion/fcntl/open.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/conversion/stat/common.h>
#include <stdbool.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/host_platform.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/link_copy.h>
#include <stddef.h>

extern char* strcpy(char* dst, const char* src);

static int copy_for_link(int dfd, const char* path, int dfdlink, const char* link, int linux_flags)
{
	struct linux_stat st;
	int from, to, ret;

	from = LINUX_SYSCALL(__NR_openat, dfd, path, link_copy_source_flags(linux_flags));
	if (from < 0)
		return from;
	ret = LINUX_SYSCALL(__NR_fstat, from, &st);
	if (ret == 0 && (st.st_mode & 0170000) != 0100000)
		ret = -LINUX_EPERM;
	if (ret < 0) {
		LINUX_SYSCALL(__NR_close, from);
		return ret;
	}
	to = LINUX_SYSCALL(__NR_openat, dfdlink, link, LINUX_O_WRONLY | LINUX_O_CREAT | LINUX_O_EXCL | LINUX_O_CLOEXEC,
		link_copy_mode(st.st_mode));
	if (to < 0) {
		LINUX_SYSCALL(__NR_close, from);
		return to;
	}
	for (;;) {
		long n = LINUX_SYSCALL(__NR_sendfile, to, from, NULL, 1 << 20);
		if (n <= 0) {
			ret = n;
			break;
		}
	}
	LINUX_SYSCALL(__NR_close, from);
	LINUX_SYSCALL(__NR_close, to);
	if (ret < 0)
		LINUX_SYSCALL(__NR_unlinkat, dfdlink, link, 0);
	return ret;
}

long sys_linkat(int fd, const char* path, int fdlink, const char* link, int flag)
{
	int ret;
	struct vchroot_expand_args vc, vc2;
	int linux_flags;

	vc.flags = (flag & BSD_AT_SYMLINK_FOLLOW) ? VCHROOT_FOLLOW : 0;
	vc.dfd = atfd(fd);

	strcpy(vc.path, path);

	vc2.flags = 0;
	vc2.dfd = atfd(fdlink);

	strcpy(vc2.path, link);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	ret = vchroot_expand(&vc2);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	linux_flags = atflags_bsd_to_linux(flag);
	if (linux_flags == LINUX_AT_INVALID)
		return -EINVAL;

	ret = LINUX_SYSCALL(__NR_linkat, vc.dfd, vc.path, vc2.dfd, vc2.path, linux_flags);
	/* QUIRK: on Android a hard link the host's SELinux policy refuses
	 * (EACCES/EPERM, for an application's own data) becomes a copy of the
	 * regular file: both names then have the same contents, which is what
	 * callers that link to keep a backup need (dpkg does). A name that is
	 * taken stays the link's own EEXIST; any other failure reports the
	 * original refusal. Evidence: contributor observation on Android, in line
	 * with Android's documented SELinux app-data policy (tier 2, not
	 * reproduced by us). The copy is not a link: later writes to one name do
	 * not show in the other, which is a deliberate deviation from link(2).
	 * Only active when host_platform_is_android(); on other hosts the host
	 * kernel's result is returned unchanged.
	 * Test: linkat-hardlink.c (pins that a normal host still gets a real
	 * hard link; the Android branch cannot be exercised off Android). */
	if ((ret == -LINUX_EACCES || ret == -LINUX_EPERM) && host_platform_is_android()) {
		int copied = copy_for_link(vc.dfd, vc.path, vc2.dfd, vc2.path, linux_flags);
		/* A name that is taken is the link's own error; otherwise report the refusal. */
		if (copied == 0 || copied == -LINUX_EEXIST)
			ret = copied;
	}
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
