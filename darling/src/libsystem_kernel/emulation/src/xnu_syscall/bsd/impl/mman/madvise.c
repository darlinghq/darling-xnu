#include <darling/emulation/xnu_syscall/bsd/impl/mman/madvise.h>

#include <sys/errno.h>

#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/common/string.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/sysctl_proc.h>

#include <stddef.h>

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

// Linux splits a transparent huge page when MADV_FREE covers only part of it,
// and libmalloc gives the free hints for single 4 KiB pages of regions that
// would otherwise stay backed by huge pages. When the host backs all anonymous
// memory with huge pages ("always"), only whole huge pages in the range are
// freed, which keeps the others intact; the hint is advisory, so freeing less
// than the range is within the contract. With "madvise" or "never" this layer
// never gets huge pages (it doesn't ask for them), and the range is passed on whole.
//
// The policy is read once per process; 0 means "pass the range on whole".
static unsigned long free_granule = (unsigned long) -1;

static unsigned long huge_page_free_granule(void)
{
	unsigned long granule = __atomic_load_n(&free_granule, __ATOMIC_RELAXED);
	char buffer[64];
	const char* selected;

	if (granule != (unsigned long) -1)
		return granule;

	granule = 0;
	if (read_string("/Volumes/SystemRoot/sys/kernel/mm/transparent_hugepage/enabled", buffer, sizeof(buffer)))
	{
		// the active policy is the one in brackets
		for (selected = buffer; *selected != '\0' && *selected != '['; ++selected)
			;
		if (strncmp(selected, "[always]", 8) == 0)
		{
			granule = 2 * 1024 * 1024;
			if (read_string("/Volumes/SystemRoot/sys/kernel/mm/transparent_hugepage/hpage_pmd_size", buffer, sizeof(buffer)))
			{
				unsigned long size = __simple_atoi(buffer, NULL);
				if (size >= 4096 && (size & (size - 1)) == 0)
					granule = size;
			}
		}
	}

	__atomic_store_n(&free_granule, granule, __ATOMIC_RELAXED);
	return granule;
}

// Whether all of [start, start + len) is mapped; Linux's madvise() fails with
// ENOMEM otherwise, and so must a hint that skips part of the range.
static int range_mapped(unsigned long start, unsigned long len)
{
	unsigned char residency[512];

	while (len > 0)
	{
		unsigned long chunk = sizeof(residency) * 4096;

		if (chunk > len)
			chunk = len;
		if (LINUX_SYSCALL(__NR_mincore, start, chunk, residency) == -LINUX_ENOMEM)
			return 0;
		start += chunk;
		len -= chunk;
	}

	return 1;
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

		unsigned long granule = huge_page_free_granule();
		if (granule != 0)
		{
			// only the whole huge pages in the range
			unsigned long first = (unsigned long) addr;
			unsigned long last = first + len;
			unsigned long start = (first + granule - 1) & ~(granule - 1);
			unsigned long end = last & ~(granule - 1);

			if (end <= start)
				start = end = last;

			// the parts that are left out still have to be mapped
			if (!range_mapped(first, start - first) || !range_mapped(end, last - end))
				ret = -LINUX_ENOMEM;
			else
				ret = 0;

			if (end > start)
			{
				int freed = LINUX_SYSCALL(__NR_madvise, start, end - start, LINUX_MADV_FREE);
				if (freed < 0)
					ret = freed;
			}
		}
		else
		{
			ret = LINUX_SYSCALL(__NR_madvise, addr, len, LINUX_MADV_FREE);
		}

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
