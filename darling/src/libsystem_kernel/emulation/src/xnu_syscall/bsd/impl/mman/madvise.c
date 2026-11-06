#include <darling/emulation/xnu_syscall/bsd/impl/mman/madvise.h>

#include <sys/errno.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>

#define MADV_FREE 5
#define MADV_FREE_REUSABLE 7
#define MADV_FREE_REUSE 8

#define LINUX_MADV_FREE 8
#define LINUX_MADV_POPULATE_WRITE 23

// MADV_POPULATE_WRITE needs Linux 5.14. Probe once with an empty range:
// Linux validates the advice before it returns 0 for len == 0.
static int populate_write_supported(void)
{
	static int supported = -1;

	if (supported < 0)
		supported = LINUX_SYSCALL(__NR_madvise, 0, 0, LINUX_MADV_POPULATE_WRITE) == 0;
	return supported;
}

long sys_madvise(void* addr, unsigned long len, int advice)
{
	int ret;

	if (advice == MADV_FREE_REUSE)
	{
		// XNU keeps reused pages from being discarded from now on. Under
		// Linux MADV_FREE only a write does that; a write prefault cancels
		// the lazy free without changing what the pages contain.
		if (!populate_write_supported())
			return 0;
		ret = LINUX_SYSCALL(__NR_madvise, addr, len, LINUX_MADV_POPULATE_WRITE);
		if (ret == -LINUX_ENOMEM)
			return errno_linux_to_bsd(ret);
		// XNU's reuse does not fault pages in, so it succeeds where
		// populating cannot (read-only or special mappings).
		return 0;
	}

	if (advice == MADV_FREE || advice == MADV_FREE_REUSABLE)
	{
		// Without a way to undo it on MADV_FREE_REUSE, a lazily freed page
		// could still be zeroed after the caller reused it; keep the hint
		// a no-op then.
		if (advice == MADV_FREE_REUSABLE && !populate_write_supported())
			return 0;
		ret = LINUX_SYSCALL(__NR_madvise, addr, len, LINUX_MADV_FREE);
		// Linux only lazily frees private anonymous memory (and only since
		// 4.5); the hint is advisory, so other mappings succeed as before.
		if (ret == -LINUX_EINVAL)
			return 0;
		if (ret < 0)
			ret = errno_linux_to_bsd(ret);
		return ret;
	}

	// advice < 0 are identical between OS X and Linux
	// advice >= 5 are specific to OS X/BSD
	if (advice >= 5)
		return 0;

	ret = LINUX_SYSCALL(__NR_madvise, addr, len, advice);

	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
