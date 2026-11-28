#include <darling/emulation/xnu_syscall/bsd/impl/unistd/renameatx_np.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>

extern char* strcpy(char* dst, const char* src);

// bsd/sys/stdio.h and bsd/man/man2/rename.2.
#define DARWIN_RENAME_SECLUDE 0x00000001
#define DARWIN_RENAME_SWAP    0x00000002
#define DARWIN_RENAME_EXCL    0x00000004
// Newer flags of <sys/stdio.h> (macOS 26 SDK): NOFOLLOW_ANY refuses any symlink in the path, RESOLVE_BENEATH
// refuses an absolute path or one that leaves the starting directory. Observed on macOS 26.7.3: with plain
// directories both succeed, an absolute path with RESOLVE_BENEATH fails with ENOTCAPABLE, a symlink in the path
// with NOFOLLOW_ANY fails with ELOOP. Neither refusal is enforced here (they only narrow what a call may reach, so
// accepting the call is the permissive reading); SECLUDE is a namespace hint with no effect.
#define DARWIN_RENAME_NOFOLLOW_ANY    0x00000010
#define DARWIN_RENAME_RESOLVE_BENEATH 0x00000020

// linux/fs.h.
#define LINUX_RENAME_NOREPLACE 0x00000001
#define LINUX_RENAME_EXCHANGE  0x00000002

long sys_renameatx_np(int fromfd, const char* from, int tofd, const char* to, unsigned int flags)
{
	struct vchroot_expand_args vc, vc2;
	unsigned int linux_flags = 0;
	long ret;

	if (from == NULL || to == NULL)
		return -EFAULT;

	// Only the two atomic variants have a Linux mapping; the other known flags are accepted and need no kernel
	// support. QUIRK: a bit outside all of them (0x8, 0x40, ...) is EINVAL, not ENOTSUP: undocumented, observed
	// on macOS 26.7.3 (tier 2). Test: libsystem_kernel_path_flags.c (renamex_np unknown flag).
	if (flags & ~(DARWIN_RENAME_SECLUDE | DARWIN_RENAME_SWAP | DARWIN_RENAME_EXCL |
	              DARWIN_RENAME_NOFOLLOW_ANY | DARWIN_RENAME_RESOLVE_BENEATH))
		return -EINVAL;
	if ((flags & DARWIN_RENAME_SWAP) && (flags & DARWIN_RENAME_EXCL))
		return -EINVAL;

	if (flags & DARWIN_RENAME_SWAP)
		linux_flags = LINUX_RENAME_EXCHANGE;
	else if (flags & DARWIN_RENAME_EXCL)
		linux_flags = LINUX_RENAME_NOREPLACE;

	vc.flags = 0;
	vc.dfd = atfd(fromfd);
	strcpy(vc.path, from);
	vc2.flags = 0;
	vc2.dfd = atfd(tofd);
	strcpy(vc2.path, to);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);
	ret = vchroot_expand(&vc2);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	if (linux_flags == 0)
	{
		// Behave like renameat() on hosts without renameat2() support too.
		ret = LINUX_SYSCALL(__NR_renameat, vc.dfd, vc.path, vc2.dfd, vc2.path);
	}
	else
	{
		ret = LINUX_SYSCALL(__NR_renameat2, vc.dfd, vc.path, vc2.dfd, vc2.path, linux_flags);
		if (ret == -LINUX_EOPNOTSUPP || ret == -LINUX_ENOSYS)
			return -ENOTSUP;
	}

	return ret < 0 ? errno_linux_to_bsd(ret) : 0;
}
