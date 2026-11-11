#include <darling/emulation/xnu_syscall/bsd/impl/time/utimes.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/time/utimes.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>

long sys_utimes(const char* path, struct bsd_timeval* tv)
{
	int ret;

	struct vchroot_expand_args vc;

	vc.flags = VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();
	
	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	// utimensat on every architecture (arm64 has no utimes syscall), with
	// the microseconds converted to nanoseconds.
	struct linux_utimensat_timespec times[2];
	if (tv)
		utimes_timeval_to_timespec(tv, times);

	ret = LINUX_SYSCALL(__NR_utimensat, LINUX_AT_FDCWD, vc.path, tv ? times : 0, 0);
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
