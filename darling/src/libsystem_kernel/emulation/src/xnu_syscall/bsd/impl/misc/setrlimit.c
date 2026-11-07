#include <darling/emulation/xnu_syscall/bsd/impl/misc/setrlimit.h>

#include <sys/errno.h>
#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

// QUIRK: setrlimit(RLIMIT_NOFILE) accepts RLIM_INFINITY as the soft limit.
// The setrlimit(2) man page (COMPATIBILITY) says it fails with EINVAL, but
// black-box runs on macOS 27.0.1 arm64 (tier 2) return 0 for
// {RLIM_INFINITY, RLIM_INFINITY}, {OPEN_MAX, RLIM_INFINITY} and soft limits
// above OPEN_MAX while the hard limit is the default RLIM_INFINITY. They
// return EINVAL only when the soft limit exceeds the requested hard limit, and
// EPERM (as non-root) for a hard limit of RLIM_INFINITY once the hard limit
// has been lowered.
// Linux cannot store RLIM_INFINITY for RLIMIT_NOFILE (EPERM above
// fs.nr_open), and sys_getrlimit never reports more than OPEN_MAX, so a
// Darwin value of OPEN_MAX or more is indistinguishable from the host's hard
// limit as long as that is at least OPEN_MAX. Such values are set to the
// host's hard limit. They must not lower the host's soft limit either: the
// descriptors Darling reserves for each thread sit just below the soft limit
// the process started with, and new threads can't get theirs above a lowered
// limit. With a host hard limit below OPEN_MAX, RLIM_INFINITY is a raise and
// Linux's EPERM stands.
// Test: rlimit_edge_cases.c, test_quirk_nofile_infinity().
static int nofile_bsd_to_linux(const struct xnu_rlimit* rlp, struct xnu_rlimit* lim)
{
	struct xnu_rlimit host;
	int ret;

	if (rlp->rlim_cur < DARWIN_OPEN_MAX && rlp->rlim_max < DARWIN_OPEN_MAX)
		return 0;

	ret = LINUX_SYSCALL(__NR_prlimit64, 0, LINUX_RLIMIT_NOFILE, 0, &host);
	if (ret < 0)
		return ret;

	if (host.rlim_max < DARWIN_OPEN_MAX)
		return 0;

	if (rlp->rlim_cur >= DARWIN_OPEN_MAX)
		lim->rlim_cur = host.rlim_max;
	if (rlp->rlim_max >= DARWIN_OPEN_MAX)
		lim->rlim_max = host.rlim_max;
	return 0;
}

long sys_setrlimit(unsigned int which, const struct xnu_rlimit* rlp)
{
	int ret;
	struct xnu_rlimit lim;

	if (rlp == NULL)
		return -EFAULT;

	lim.rlim_cur = rlim_bsd_to_linux(rlp->rlim_cur);
	lim.rlim_max = rlim_bsd_to_linux(rlp->rlim_max);

	which = rlimit_bsd_to_linux(which);
	if (which == -1)
		return -EINVAL;

	// Checked on the Darwin values: the infinity mapping below can make a
	// soft limit above the hard limit look valid.
	if (rlp->rlim_cur > rlp->rlim_max)
		return -EINVAL;

	if (which == LINUX_RLIMIT_NOFILE)
	{
		ret = nofile_bsd_to_linux(rlp, &lim);
		if (ret < 0)
			return errno_linux_to_bsd(ret);
	}

	ret = LINUX_SYSCALL(__NR_prlimit64, 0, which, &lim, 0);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
