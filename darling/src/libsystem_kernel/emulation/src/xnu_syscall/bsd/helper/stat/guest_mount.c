// Copyright (C) 2026 Lubos Dolezel
#include <darling/emulation/xnu_syscall/bsd/helper/stat/guest_mount.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/mount_table.h>
#include <darling/emulation/xnu_syscall/bsd/helper/stat/linux_statfs.h>
#include <darling/emulation/xnu_syscall/bsd/helper/stat/mount_options.h>
#include <darling/emulation/conversion/common_at.h>

extern __SIZE_TYPE__ strlen(const char* s);
extern int strcmp(const char* a, const char* b);
extern int strncmp(const char* a, const char* b, __SIZE_TYPE__ n);
extern char* strcpy(char* dst, const char* src);
extern __SIZE_TYPE__ strlcpy(char* dst, const char* src, __SIZE_TYPE__ size);

// <sys/mount.h> values
#define BSD_MNT_DONTBROWSE 0x00100000
#define BSD_MNT_LOCAL      0x00001000
#define BSD_MNT_REMOVABLE  0x00000200
#define BSD_MNT_ROOTFS     0x00004000

// the host's tree below the guest's /Volumes (the vchroot "exit path")
#define HOST_ROOT_ALIAS "/Volumes/SystemRoot"

struct guest_root
{
	char path[4096];        // host path of the guest's "/", without a trailing slash ("" for a host root)
	size_t length;
	char volume[1024];      // host mount point of the root volume
};

static long guest_root_find(struct guest_root* root)
{
	struct vchroot_expand_args args;
	char fstype[16];
	char from[8];
	int ret;

	args.flags = VCHROOT_FOLLOW;
	args.dfd = -100;
	strcpy(args.path, "/");
	ret = vchroot_expand(&args);
	if (ret < 0)
		return errno_linux_to_bsd(ret);

	root->length = strlen(args.path);
	while (root->length > 0 && args.path[root->length - 1] == '/')
		root->length--;
	args.path[root->length] = 0;
	strcpy(root->path, args.path);

	return mount_table_lookup(args.path[0] ? args.path : "/", root->volume, sizeof(root->volume),
		fstype, sizeof(fstype), from, sizeof(from));
}

// the name the guest gives to a host mount point; `listed` tells whether the guest can name the mount at all
static void guest_mount_name(const struct guest_root* root, const char* mnton, char* name, size_t name_size, int* listed)
{
	if (strcmp(mnton, root->volume) == 0)
	{
		strlcpy(name, "/", name_size);
		*listed = 1;
	}
	else if (strncmp(mnton, root->path, root->length) == 0 && mnton[root->length] == '/')
	{
		strlcpy(name, mnton + root->length, name_size);
		*listed = 1;
	}
	else
	{
		strlcpy(name, HOST_ROOT_ALIAS, name_size);
		if (!(mnton[0] == '/' && mnton[1] == 0))
			strlcpy(name + sizeof(HOST_ROOT_ALIAS) - 1, mnton, name_size - (sizeof(HOST_ROOT_ALIAS) - 1));
		*listed = 0;
	}
}

// A FUSE file system may name its kind (e.g. "hfs" for darling-dmg) in the "darling.volume.kind" xattr of its root;
// that kind replaces the Linux type ("fuse" or "fuse.<subtype>"), as macOS reports the file system in the volume.
// Returns 1 when it did.
static int guest_mount_kind(const char* mnton, char* fstype, size_t fstype_size)
{
	char kind[16];
	long ret;

	if (strncmp(fstype, "fuse", 4) != 0)
		return 0;
	ret = LINUX_SYSCALL(__NR_lgetxattr, mnton, "darling.volume.kind", kind, sizeof(kind) - 1);
	if (ret <= 0)
		return 0;
	kind[ret] = 0;
	strlcpy(fstype, kind, fstype_size);
	return 1;
}

// The mount flags macOS gives a volume and Linux has no counterpart for:
// - MNT_LOCAL for every volume that is not a network file system;
// - MNT_ROOTFS for the root volume;
// - MNT_DONTBROWSE for a volume the user is not meant to see, i.e. one that is neither "/" nor in /Volumes;
// - MNT_REMOVABLE for a disk image, which is a volume whose kind comes from its marker (as in diskarbitrationd).
static unsigned int guest_mount_flags(const struct guest_root* root, const char* mnton, const char* guest_name,
	const char* linux_fstype, int has_kind)
{
	static const char* const network_types[] = { "nfs", "nfs4", "cifs", "smb3", "9p", "ceph", "fuse.sshfs" };
	unsigned int flags = BSD_MNT_LOCAL;
	int is_root = strcmp(mnton, root->volume) == 0;

	for (size_t i = 0; i < sizeof(network_types) / sizeof(network_types[0]); i++)
	{
		if (strcmp(linux_fstype, network_types[i]) == 0)
			flags = 0;
	}
	if (is_root)
		flags |= BSD_MNT_ROOTFS;
	else
	{
		const char* leaf = guest_name + sizeof("/Volumes/") - 1;
		int in_volumes = strncmp(guest_name, "/Volumes/", sizeof("/Volumes/") - 1) == 0 && *leaf != 0;

		for (const char* p = leaf; in_volumes && *p; p++)
		{
			if (*p == '/')
				in_volumes = 0;
		}
		if (!in_volumes)
			flags |= BSD_MNT_DONTBROWSE;
	}
	if (has_kind)
		flags |= BSD_MNT_REMOVABLE;
	return flags;
}

long guest_mount_volume_name(const char* host_path, char* name, __SIZE_TYPE__ name_size)
{
	struct guest_root root;
	char mnton[1024], fstype[16], from[8], guest_name[1024];
	const char* last;
	int listed;
	long ret = guest_root_find(&root);

	if (ret < 0)
		return ret;
	ret = mount_table_lookup(host_path, mnton, sizeof(mnton), fstype, sizeof(fstype), from, sizeof(from));
	if (ret < 0)
		return ret;

	if (mnton[0] != 0 && strncmp(fstype, "fuse", 4) == 0 && name_size > 1)
	{
		ret = LINUX_SYSCALL(__NR_lgetxattr, mnton, "darling.volume.name", name, name_size - 1);
		if (ret > 0)
		{
			name[ret] = 0;
			return 0;
		}
	}

	if (mnton[0] == 0 || strcmp(mnton, root.volume) == 0)
	{
		strlcpy(name, "Macintosh HD", name_size);
		return 0;
	}

	guest_mount_name(&root, mnton, guest_name, sizeof(guest_name), &listed);
	last = guest_name;
	for (const char* p = guest_name; *p; p++)
	{
		if (*p == '/' && p[1] != 0)
			last = p + 1;
	}
	strlcpy(name, last, name_size);
	return 0;
}

// The user who mounted a volume: for a FUSE mount the user_id Linux records (in the guest's user namespace, as
// /proc/self/mounts shows it), otherwise root, as on macOS for the volumes the system mounts.
static unsigned int guest_mount_owner(const char* fstype, const char* options)
{
	unsigned int owner = 0;

	if (strncmp(fstype, "fuse", 4) == 0)
		mount_options_fuse_user_id(options, &owner);
	return owner;
}

// QUIRK: f_fsid's first value is the volume's device number, the st_dev that stat() reports for the files on it, so no
// two mounted volumes share it. statfs(2) only calls it the file system ID. Evidence: black-box observation on macOS
// 26.7.3 and 10.14.4, x86_64 (tier 2), on APFS, HFS+ and devfs volumes. Linux's f_fsid can't stand in for it: an
// overlay reports the one of the file system under it, so the guest's root and the host's root looked like one volume.
// Test: darling/tests/statfs-fsid.c (run by the darling repo's tests/volume-attributes.sh)
static void guest_mount_set_fsid(const char* host_path, struct bsd_statfs64* buf)
{
	struct linux_stat lstat;
	int ret;

#ifdef __NR_newfstatat
	ret = LINUX_SYSCALL(__NR_newfstatat, LINUX_AT_FDCWD, host_path, &lstat, 0);
#else
	ret = LINUX_SYSCALL(__NR_fstatat64, LINUX_AT_FDCWD, host_path, &lstat, 0);
#endif
	if (ret == 0)
		buf->f_fsid = (buf->f_fsid & 0xffffffff00000000ULL) | (unsigned int) lstat.st_dev; // val[0] is the low half
}

long guest_mount_describe(const char* host_path, struct bsd_statfs64* buf)
{
	struct guest_root root;
	char mnton[1024];
	int listed;
	long ret = guest_root_find(&root);

	if (ret < 0)
		return ret;

	ret = mount_table_lookup(host_path, mnton, sizeof(mnton),
		buf->f_fstypename, sizeof(buf->f_fstypename),
		buf->f_mntfromname, sizeof(buf->f_mntfromname));
	if (ret < 0)
		return ret;

	if (mnton[0] != 0)
	{
		char fstype[sizeof(buf->f_fstypename)];
		char options[1024];

		buf->f_owner = 0;
		if (mount_table_mount_at(mnton, fstype, sizeof(fstype), options, sizeof(options)) == 0)
			buf->f_owner = guest_mount_owner(fstype, options);
		guest_mount_name(&root, mnton, buf->f_mntonname, sizeof(buf->f_mntonname), &listed);
		strlcpy(fstype, buf->f_fstypename, sizeof(fstype));
		buf->f_flags |= guest_mount_flags(&root, mnton, buf->f_mntonname, fstype,
			guest_mount_kind(mnton, buf->f_fstypename, sizeof(buf->f_fstypename)));
		guest_mount_set_fsid(mnton, buf);
	}
	else
	{
		buf->f_mntonname[0] = 0;
		guest_mount_set_fsid(host_path, buf);
	}
	return 0;
}

struct list_state
{
	const struct guest_root* root;
	void* buf;         // struct bsd_statfs64 entries, or struct bsd_statfs entries if `legacy`
	int legacy;
	int capacity;
	int count;
	int root_pass;   // 1: only the root volume, 0: everything else
};

static long list_visit(void* context, const char* mnton, const char* fstype, const char* mntfrom,
	const char* options)
{
	struct list_state* state = context;
	struct linux_statfs64 lbuf;
	char name[1024];
	int listed;
	int is_root = strcmp(mnton, state->root->volume) == 0;

	if (is_root != state->root_pass)
		return 0;

	guest_mount_name(state->root, mnton, name, sizeof(name), &listed);
	if (!listed)
		return 0;

	// every mount is queried, with or without a buffer, so that the count and the list agree
	if (linux_statfs_path(mnton, &lbuf) < 0)
		return 0;

	if (state->buf != NULL)
	{
		struct bsd_statfs64 wide;
		struct bsd_statfs64* entry = state->legacy ? &wide : &((struct bsd_statfs64*) state->buf)[state->count];

		if (state->count >= state->capacity)
			return 1;

		statfs_linux_to_bsd64(&lbuf, entry);
		strlcpy(entry->f_mntonname, name, sizeof(entry->f_mntonname));
		strlcpy(entry->f_fstypename, fstype, sizeof(entry->f_fstypename));
		entry->f_flags |= guest_mount_flags(state->root, mnton, name, fstype,
			guest_mount_kind(mnton, entry->f_fstypename, sizeof(entry->f_fstypename)));
		strlcpy(entry->f_mntfromname, mntfrom, sizeof(entry->f_mntfromname));
		entry->f_owner = guest_mount_owner(fstype, options);
		guest_mount_set_fsid(mnton, entry);
		if (state->legacy)
			statfs_bsd64_to_bsd(&wide, &((struct bsd_statfs*) state->buf)[state->count]);
	}

	state->count++;
	return 0;
}

static long guest_mount_list_common(void* buf, int legacy, int capacity)
{
	struct guest_root root;
	struct list_state state = { .root = &root, .buf = buf, .legacy = legacy, .capacity = capacity };
	long ret = guest_root_find(&root);

	if (ret < 0)
		return ret;

	for (state.root_pass = 1; state.root_pass >= 0; state.root_pass--)
	{
		ret = mount_table_enumerate(list_visit, &state);
		if (ret < 0)
			return ret;
	}
	return state.count;
}

long guest_mount_list(struct bsd_statfs64* buf, int bufsize)
{
	return guest_mount_list_common(buf, 0, bufsize / (int) sizeof(struct bsd_statfs64));
}

long guest_mount_list_legacy(struct bsd_statfs* buf, int bufsize)
{
	return guest_mount_list_common(buf, 1, bufsize / (int) sizeof(struct bsd_statfs));
}
