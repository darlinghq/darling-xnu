#include <darling/emulation/legacy_path/mman/msync.h>

#include <darling/emulation/legacy_path/mman/duct_mman.h>
#include <darling/emulation/legacy_path/errno.h>
#include <darling/emulation/legacy_path/base.h>
#include <darling/emulation/legacy_path/linux-syscalls/linux.h>
#include <darling/emulation/legacy_path/bsdthread/cancelable.h>

int msync_flags_bsd_to_linux(int flags)
{
	int oflags = 0;

	if (flags & BSD_MS_ASYNC)
		oflags |= LINUX_MS_ASYNC;
	if (flags & BSD_MS_SYNC)
		oflags |= LINUX_MS_SYNC;
	if (flags & BSD_MS_INVALIDATE)
		oflags |= LINUX_MS_INVALIDATE;

	return oflags;
}

long sys_msync(void* addr, unsigned long len, int flags)
{
	CANCELATION_POINT();
	return sys_msync_nocancel(addr, len, flags);
}

long sys_msync_nocancel(void* addr, unsigned long len, int flags)
{
	int ret;

	ret = LINUX_SYSCALL(__NR_msync, addr, len, msync_flags_bsd_to_linux(flags));
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
