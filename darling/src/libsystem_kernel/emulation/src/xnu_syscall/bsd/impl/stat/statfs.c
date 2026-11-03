#include <darling/emulation/xnu_syscall/bsd/impl/stat/statfs.h>

#include <stddef.h>
#include <sys/fcntl.h>
#include <sys/errno.h>

#include <darling/emulation/conversion/stat/common.h>
#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/common/simple.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/common/bsdthread/per_thread_wd.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/helper/stat/guest_mount.h>
#include <darling/emulation/xnu_syscall/bsd/helper/stat/linux_statfs.h>

#undef strlcpy

extern char *strtok_r(char *str, const char *delim, char **saveptr);
extern __SIZE_TYPE__ strlcpy(char* dst, const char* src, __SIZE_TYPE__ size);
extern __SIZE_TYPE__ strlen(const char* str);
extern int strncmp(const char* s1, const char* s2, __SIZE_TYPE__ len);

long sys_statfs(const char* path, struct bsd_statfs* buf)
{
	// the legacy structure (every architecture has one, with the long fields of its own width)
	struct bsd_statfs64 wide;
	long ret = sys_statfs64(path, &wide);

	if (ret < 0)
		return ret;
	statfs_bsd64_to_bsd(&wide, buf);
	return 0;
#endif
}
}

// btrfs subvolumes (and other st_dev changes) that are not mount points. A subvolume reached
// through its parent ("/srv/projects" under the "/" mount) has its own st_dev but no /proc/self/mounts
// line, so its files' fsid {st_dev,0} (stat, getattrlist ATTR_CMN_FSID) matched no getfsstat entry:
// CFURL's NSURLVolumeURLKey could not match such a file to a volume. Report the subvolume as a
// volume of its own, mounted on its root: the topmost directory
// with the file's st_dev. statfs/fstatfs return it and getfsstat lists it (the prefix's subvolume, and
// every one statfs/fstatfs found in this process).
int statfs_subvol_root(const char* linux_path, unsigned long long dev, char* out, unsigned long outsize)
{
	char cur[1024];
	int len;

	if (strlcpy(cur, linux_path, sizeof(cur)) >= sizeof(cur) || cur[0] != '/')
		return -1;
	len = strlen(cur);
	while (len > 1 && cur[len - 1] == '/')
		cur[--len] = 0;

	while (len > 1)
	{
		struct linux_stat pst;
		char saved;
		int plen = len;

		while (plen > 0 && cur[plen - 1] != '/')
			plen--;
		if (plen > 1)
			plen--; // drop the '/' unless the parent is "/"
		saved = cur[plen];
		cur[plen] = 0;
		if (LINUX_SYSCALL(__NR_stat, cur, &pst) != 0 || pst.st_dev != dev)
		{
			cur[plen] = saved;
			break;
		}
		len = plen;
	}
	strlcpy(out, cur, outsize);
	return 0;
}

#define STATFS_MAX_SUBVOLS 16
static char subvol_roots[STATFS_MAX_SUBVOLS][1024];
static unsigned long long subvol_devs[STATFS_MAX_SUBVOLS];
static int subvol_count; // entries [0, subvol_count) are complete

static void statfs_remember_subvol(const char* root, unsigned long long dev)
{
	int n = __atomic_load_n(&subvol_count, __ATOMIC_ACQUIRE);
	for (int i = 0; i < n; i++)
		if (subvol_devs[i] == dev)
			return;
	// Racing threads may both add the same root; getfsstat drops duplicate devs.
	n = __atomic_load_n(&subvol_count, __ATOMIC_ACQUIRE);
	if (n >= STATFS_MAX_SUBVOLS)
		return;
	strlcpy(subvol_roots[n], root, sizeof(subvol_roots[n]));
	subvol_devs[n] = dev;
	__atomic_compare_exchange_n(&subvol_count, &n, n + 1, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
}

// Index 0: the subvolume of the prefix root (the guest's "/"), always looked up, so a fresh process
// lists it without a statfs first. Then those statfs/fstatfs met. Returns -1 past the end.
int statfs_extra_root(int i, char* out, unsigned long outsize, unsigned long long* dev)
{
	if (i == 0)
	{
		struct vchroot_expand_args vc;
		struct linux_stat st;
		vc.flags = VCHROOT_FOLLOW;
		vc.dfd = get_perthread_wd();
		strcpy(vc.path, "/");
		if (vchroot_expand(&vc) < 0 || LINUX_SYSCALL(__NR_stat, vc.path, &st) != 0
			|| statfs_subvol_root(vc.path, st.st_dev, out, outsize) != 0)
		{
			out[0] = 0;
			*dev = 0;
		}
		else
			*dev = st.st_dev;
		return 0;
	}
	i--;
	if (i >= __atomic_load_n(&subvol_count, __ATOMIC_ACQUIRE))
		return -1;
	strlcpy(out, subvol_roots[i], outsize);
	*dev = subvol_devs[i];
	return 0;
}


long sys_statfs64(const char* path, struct bsd_statfs64* buf)
{
	int ret;
	struct linux_statfs64 lbuf;

	struct vchroot_expand_args vc;
	vc.flags = VCHROOT_FOLLOW;
	vc.dfd = get_perthread_wd();

	// statfs(NULL) is EFAULT on Darwin; strcpy from NULL would kill the caller
	if (!path)
		return -EFAULT;
	strcpy(vc.path, path);
	ret = vchroot_expand(&vc);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	ret = linux_statfs_path(vc.path, &lbuf);

	if (ret < 0)
		return errno_linux_to_bsd(ret);

	statfs_linux_to_bsd64(&lbuf, buf);

	return guest_mount_describe(vc.path, buf);
}
