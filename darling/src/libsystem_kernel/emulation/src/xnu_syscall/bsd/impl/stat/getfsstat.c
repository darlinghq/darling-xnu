#include <darling/emulation/xnu_syscall/bsd/impl/stat/getfsstat.h>

#include <stddef.h>

#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/xnu_syscall/bsd/helper/stat/guest_mount.h>

long sys_getfsstat(struct bsd_statfs* buf, int bufsize, int flags)
{
	// the legacy structure; bufsize counts bytes of it
	return guest_mount_list_legacy(buf, bufsize);
}

long sys_getfsstat64(struct bsd_statfs64* buf, int bufsize, int flags)
{
	// the volumes the guest can use, named as the guest sees them, root volume first
	return guest_mount_list(buf, bufsize);
}
