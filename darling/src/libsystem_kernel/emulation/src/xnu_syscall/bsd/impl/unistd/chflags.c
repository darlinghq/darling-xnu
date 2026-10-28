#include <darling/emulation/xnu_syscall/bsd/impl/unistd/chflags.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/xnu_syscall/bsd/helper/misc/bsdflags.h>


// The flags are kept in an extended attribute (helper/misc/bsdflags.h); this used to fail with ENOTSUP.
long sys_chflags(const char* file, int flags)
{
	struct vchroot_expand_args vc;
	vc.flags = VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();
	strcpy(vc.path, file);
	long r = vchroot_expand(&vc);
	if (r < 0)
		return errno_linux_to_bsd(r);
	char buf[1100];
	const char* p = vc.path;
	if (vc.path[0] != '/' && vc.dfd != -100)
	{
		__simple_snprintf(buf, sizeof(buf), "/proc/self/fd/%d/%s", vc.dfd, vc.path);
		p = buf;
	}
	r = darling_bsdflags_bsdflags_set(p, (uint32_t) flags);
	return r < 0 ? errno_linux_to_bsd(r) : 0;
}
