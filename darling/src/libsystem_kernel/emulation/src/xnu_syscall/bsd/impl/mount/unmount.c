#include <darling/emulation/xnu_syscall/bsd/impl/mount/unmount.h>

#include <stddef.h>
#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/duct_errno.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/conversion/common_at.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/xnu_syscall/bsd/helper/stat/mount_options.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/mount_table.h>

#ifndef VARIANT_DYLD
#include <elfcalls.h>
extern struct elf_calls* elfcalls(void);
#endif

extern __SIZE_TYPE__ strlen(const char* s);
extern char* strcpy(char* dst, const char* src);
extern int strcmp(const char* a, const char* b);
extern int strncmp(const char* a, const char* b, __SIZE_TYPE__ n);

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

#ifndef VARIANT_DYLD
// Where the host's setuid FUSE helper may be. The process's Linux root is the host's root in some launcher modes and
// the prefix in others, where the host's tree is below /Volumes/SystemRoot.
static const char* const fusermount_paths[] = {
	"/usr/bin/fusermount", "/bin/fusermount", "/usr/bin/fusermount3", "/bin/fusermount3",
	"/Volumes/SystemRoot/usr/bin/fusermount", "/Volumes/SystemRoot/bin/fusermount",
	"/Volumes/SystemRoot/usr/bin/fusermount3", "/Volumes/SystemRoot/bin/fusermount3",
};

// Unmounts the FUSE mount on `host_path` with the host's fusermount, which lets the user who mounted a FUSE
// filesystem unmount it without privileges. Returns 0, -EBUSY if fusermount failed, or -EPERM if it can't be run.
// With MNT_FORCE it asks for a lazy unmount (-z): detaching the busy volume is the closest an unprivileged
// unmount comes to a forced one.
static long unmount_with_fusermount(const char* host_path, int force)
{
	typedef int (*posix_spawn_fn)(int* pid, const char* path, const void* file_actions, const void* attr,
		char* const argv[], char* const envp[]);
	static char env_path[] = "PATH=/usr/bin:/bin";
	char* envp[] = { env_path, NULL };
	posix_spawn_fn spawn = (posix_spawn_fn) elfcalls()->dlsym(NULL, "posix_spawn");
	int pid = -1;
	int status;
	long ret;

	if (spawn == NULL)
		return -EPERM;

	for (size_t i = 0; i < sizeof(fusermount_paths) / sizeof(fusermount_paths[0]); i++)
	{
		const char* argv[] = { "fusermount", "-u", "-q", force ? "-z" : host_path, force ? host_path : NULL, NULL };
		int err = spawn(&pid, fusermount_paths[i], NULL, NULL, (char* const*) argv, envp);
		if (err == 0)
			break;
		pid = -1;
		if (err != LINUX_ENOENT)
			return -EPERM;
	}
	if (pid < 0)
		return -EPERM;

	do
		ret = LINUX_SYSCALL(__NR_wait4, pid, &status, 0, NULL);
	while (ret == -LINUX_EINTR);
	if (ret < 0)
		return -EPERM;

	// exited normally with status 0
	return (status & 0xffff) == 0 ? 0 : -EBUSY;
}
#endif

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

#ifndef VARIANT_DYLD
	unsigned int owner;

	// a FUSE volume can still be unmounted by the user who mounted it, as on macOS a user can unmount the volumes
	// they mounted
	if (strncmp(fstype, "fuse", 4) != 0 || !mount_options_fuse_user_id(options, &owner)
		|| owner != (unsigned int) LINUX_SYSCALL(__NR_getuid))
		return -EPERM;

	ret = unmount_with_fusermount(vc.path, (flags & BSD_MNT_FORCE) != 0);
	if (ret == -EBUSY && mount_table_mount_at(vc.path, fstype, sizeof(fstype), options, sizeof(options)) == -EINVAL)
		return 0; // gone after all
	return ret;
#else
	return -EPERM;
#endif
}
