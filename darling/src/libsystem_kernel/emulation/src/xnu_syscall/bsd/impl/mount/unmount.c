#include <darling/emulation/xnu_syscall/bsd/impl/mount/unmount.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/mount_table.h>

extern __SIZE_TYPE__ strlen(const char* s);
extern char* strcpy(char* dst, const char* src);
extern int strcmp(const char* a, const char* b);

#define BSD_MNT_FORCE 0x00080000
#define LINUX_MNT_FORCE 1

// the host path `path` expands to, without trailing slashes, as /proc/self/mounts names mount points
static long unmount_host_path(const char* path, struct vchroot_expand_args* vc)
{
	size_t length;
	int ret;

	vc->flags = VCHROOT_FOLLOW;
	vc->dfd = LINUX_AT_FDCWD;
	strcpy(vc->path, path);
	ret = vchroot_expand(vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	length = strlen(vc->path);
	while (length > 1 && vc->path[length - 1] == '/')
		vc->path[--length] = 0;
	return 0;
}

// whether `host_path` is the host path of the guest's "/"
static int unmount_is_guest_root(const char* host_path)
{
	struct vchroot_expand_args root;

	if (unmount_host_path("/", &root) < 0)
		return 0;
	return strcmp(root.path, host_path) == 0;
}

long sys_unmount(const char* path, int flags)
{
	struct vchroot_expand_args vc;
	char fstype[64];
	char options[512];
	long ret;

	if (path == NULL)
		return -EFAULT;

	ret = unmount_host_path(path, &vc);
	if (ret < 0)
		return ret;

	ret = LINUX_SYSCALL(__NR_umount2, vc.path, (flags & BSD_MNT_FORCE) ? LINUX_MNT_FORCE : 0);
	if (ret == 0)
		return 0;
	if (ret != -LINUX_EPERM && ret != -LINUX_EACCES)
		return errno_linux_to_bsd(ret);

	// Without the privilege to unmount, Linux fails before it checks that the path is a mount point. macOS checks
	// that first, so look it up.
	ret = mount_table_mount_at(vc.path, fstype, sizeof(fstype), options, sizeof(options));
	if (ret == -EINVAL && unmount_is_guest_root(vc.path))
		return -EPERM; // the root volume, even where the guest's root directory is not a mount of its own
	if (ret < 0)
		return ret;

	return -EPERM;
}
