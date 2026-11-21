// Copyright (C) 2026 Lubos Dolezel
#include <darling/emulation/xnu_syscall/bsd/helper/stat/guest_mount.h>

#include <stddef.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/vchroot_expand.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/xnu_syscall/bsd/impl/stat/mount_table.h>
#include <darling/emulation/xnu_syscall/bsd/helper/stat/linux_statfs.h>

extern __SIZE_TYPE__ strlen(const char* s);
extern int strcmp(const char* a, const char* b);
extern int strncmp(const char* a, const char* b, __SIZE_TYPE__ n);
extern char* strcpy(char* dst, const char* src);
extern __SIZE_TYPE__ strlcpy(char* dst, const char* src, __SIZE_TYPE__ size);

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
		guest_mount_name(&root, mnton, buf->f_mntonname, sizeof(buf->f_mntonname), &listed);
	else
		buf->f_mntonname[0] = 0;
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

static long list_visit(void* context, const char* mnton, const char* fstype, const char* mntfrom)
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
		strlcpy(entry->f_mntfromname, mntfrom, sizeof(entry->f_mntfromname));
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
