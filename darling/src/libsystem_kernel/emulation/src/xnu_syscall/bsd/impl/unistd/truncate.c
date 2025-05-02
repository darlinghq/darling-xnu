#include <darling/emulation/xnu_syscall/bsd/impl/unistd/truncate.h>

#include <darling/emulation/other/mach/lkm.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>

long sys_truncate(const char* path, long long length)
{
	int ret;
	struct vchroot_expand_args vc;

	vc.flags = VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();
	
	strcpy(vc.path, path);

	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

#ifdef __NR_truncate64
	ret = LINUX_SYSCALL(__NR_truncate64, vc.path, LL_ARG(length));
#else
	ret = LINUX_SYSCALL(__NR_truncate, vc.path, LL_ARG(length));
#endif
	if (ret < 0)
		ret = errno_linux_to_bsd(ret);

	return ret;
}
